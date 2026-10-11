#pragma once

#include "Event.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace GNC {

class EventBus;

namespace detail {

// The bus's listener table. Lives behind a shared_ptr so subscription tokens
// can hold a weak_ptr to it; see EventSubscription.
struct EventRegistry {
    using ListenerID = std::uint64_t;
    using ErasedListener = std::function<bool(Event&)>;

    struct ListenerEntry {
        ListenerID id;
        int priority;
        ErasedListener fn;

        // Cleared instead of erasing when unsubscribed mid-dispatch. The
        // callable itself is left intact until compaction: the listener
        // being removed may be the one currently executing, and destroying
        // its closure under it would pull the captures out from under the
        // running lambda.
        bool active = true;
    };

    std::unordered_map<EventTypeID, std::vector<ListenerEntry>> listeners;
    ListenerID nextListenerID = 1; // 0 means "no subscription"

    // Flush() walks the listener vectors by index. A listener that
    // subscribes or unsubscribes another during that walk would invalidate
    // the iteration, so while dispatchDepth > 0 additions are parked here
    // and removals only deactivate the entry; both are applied once the
    // walk is done.
    int dispatchDepth = 0;
    std::vector<std::pair<EventTypeID, ListenerEntry>> pendingAdds;
    bool needsCompaction = false;
};

} // namespace detail

// Event subscription
// RAII token returned by EventBus::Subscribe(). Destroying it (or letting it
// go out of scope) removes the listener, so an object that subscribes in its
// constructor and keeps the token as a member can never leave a dangling
// callback behind when it is destroyed. That is the classic event-system bug,
// and this is the cheapest way to make it impossible.
//
// The token only holds a weak reference to the bus's internals: a token that
// outlives the bus itself is a harmless no-op on destruction.
class EventSubscription {
private:
    friend class EventBus;

    std::weak_ptr<detail::EventRegistry> registry;
    EventTypeID typeID = 0;
    std::uint64_t listenerID = 0;

    EventSubscription(std::weak_ptr<detail::EventRegistry> reg, EventTypeID type, std::uint64_t id)
        : registry(std::move(reg)), typeID(type), listenerID(id) {}

public:
    EventSubscription() = default;
    ~EventSubscription() { Reset(); }

    EventSubscription(const EventSubscription&) = delete;
    EventSubscription& operator=(const EventSubscription&) = delete;

    EventSubscription(EventSubscription&& other) noexcept { *this = std::move(other); }
    EventSubscription& operator=(EventSubscription&& other) noexcept {
        if (this != &other) {
            Reset();
            registry = std::move(other.registry);
            typeID = other.typeID;
            listenerID = other.listenerID;
            other.registry.reset();
            other.listenerID = 0;
        }
        return *this;
    }

    bool IsActive() const { return listenerID != 0 && !registry.expired(); }

    // Unsubscribes now rather than at destruction. Defined below EventBus,
    // which owns the registry layout.
    void Reset();
};

// Event bus
// Queue-only publish/subscribe. Nothing is delivered at the moment it is
// enqueued; everything waits for Flush(), which the frame loop calls once,
// right after glfwPollEvents() and before update(). Two consequences that are
// the whole point of the design:
//
//   - a listener always runs at one known moment in the frame, never inside a
//     GLFW callback, never mid-iteration over a component array, never on the
//     hot-reload watcher thread.
//   - the publisher never learns synchronously whether its event was handled.
//     That check belongs in the listeners: they run in priority order and a
//     later one can look at IsHandled().
//
// Thread safety: Enqueue() may be called from any thread. Subscribe(),
// Unsubscribe() and Flush() are main-thread only, same as the rest of the
// engine's per-frame work.
//
// Delivery order: listeners for a type run from highest priority to lowest,
// ties in subscription order. The first one to return true marks the event
// handled and ends delivery. That ordering is the engine's "layer stack":
// an ImGui layer subscribes at a high priority and consumes input while it
// has focus; the camera subscribes lower and simply never sees those events.
class EventBus {
private:
    using Registry = detail::EventRegistry;
    using ListenerID = Registry::ListenerID;
    using ListenerEntry = Registry::ListenerEntry;

    friend class EventSubscription;

    // shared_ptr rather than a member so subscription tokens can hold a
    // weak_ptr and detect the bus dying first.
    std::shared_ptr<Registry> registry = std::make_shared<Registry>();

    std::vector<std::unique_ptr<Event>> queue;
    std::mutex queueMutex;

    // Whether Flush() should keep calling listeners after one has handled
    // the event. Off by default: "handled" means "stop", which is what the
    // layer-stack use needs. Turn it on for a debugging listener that wants
    // to observe everything.
    bool deliverHandled = false;

public:
    EventBus() = default;
    ~EventBus() = default;

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // ---- subscribing --------------------------------------------------

    // Registers a listener for events of exactly type E. The listener returns
    // true to mark the event handled and stop further delivery, false to let
    // it continue to lower-priority listeners.
    //
    // The returned token must be kept alive for as long as the listener
    // should stay registered; dropping it unsubscribes.
    template<typename E>
    [[nodiscard]] EventSubscription Subscribe(std::function<bool(E&)> listener, int priority = 0) {
        static_assert(std::is_base_of_v<Event, E>, "E must derive from GNC::Event");

        Registry& reg = *registry;
        const EventTypeID typeID = E::StaticTypeID();
        const ListenerID id = reg.nextListenerID++;

        // The static_cast is safe: the bus only ever looks up listeners by
        // the event's own GetTypeID(), so an entry stored under
        // E::StaticTypeID() is only ever invoked with an E.
        ListenerEntry entry{id, priority, [fn = std::move(listener)](Event& e) {
            return fn(static_cast<E&>(e));
        }};

        if (reg.dispatchDepth > 0) {
            reg.pendingAdds.emplace_back(typeID, std::move(entry));
        } else {
            InsertSorted(reg.listeners[typeID], std::move(entry));
        }

        return EventSubscription(registry, typeID, id);
    }

    // Convenience overload so callers can pass a lambda without spelling out
    // the std::function type.
    template<typename E, typename F>
        requires std::is_invocable_r_v<bool, F&, E&>
    [[nodiscard]] EventSubscription Subscribe(F&& listener, int priority = 0) {
        return Subscribe<E>(std::function<bool(E&)>(std::forward<F>(listener)), priority);
    }

    // ---- publishing ---------------------------------------------------

    // Constructs an E in place and queues it. Safe from any thread.
    template<typename E, typename... Args>
    void Enqueue(Args&&... args) {
        static_assert(std::is_base_of_v<Event, E>, "E must derive from GNC::Event");
        Enqueue(std::make_unique<E>(std::forward<Args>(args)...));
    }

    void Enqueue(std::unique_ptr<Event> event) {
        if (!event) return;
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push_back(std::move(event));
    }

    // Like Enqueue(), but if an event of the same type is already waiting,
    // replaces the most recent one instead of adding another. For events
    // where only the latest value matters: a window being dragged to a new
    // size fires dozens of resize events per frame, and recreating the
    // swapchain for each intermediate size would be wasted work.
    template<typename E, typename... Args>
    void EnqueueCoalesced(Args&&... args) {
        static_assert(std::is_base_of_v<Event, E>, "E must derive from GNC::Event");
        auto event = std::make_unique<E>(std::forward<Args>(args)...);

        std::lock_guard<std::mutex> lock(queueMutex);
        auto it = std::find_if(queue.rbegin(), queue.rend(), [](const auto& queued) {
            return queued->GetTypeID() == E::StaticTypeID();
        });
        if (it != queue.rend()) {
            *it = std::move(event);
        } else {
            queue.push_back(std::move(event));
        }
    }

    // ---- delivering ---------------------------------------------------

    // Delivers everything queued so far and returns how many events were
    // dispatched. Main thread only; call once per frame.
    //
    // Events enqueued *by a listener* during this call land in the next
    // Flush(), not this one. That keeps a frame's event count bounded and
    // makes ordering easy to reason about: a listener never runs before the
    // events that were queued ahead of it.
    std::size_t Flush() {
        std::vector<std::unique_ptr<Event>> batch;
        {
            // Swap under the lock, dispatch outside it: a listener that takes
            // a while must never stall a producer thread's Enqueue().
            std::lock_guard<std::mutex> lock(queueMutex);
            batch.swap(queue);
        }

        for (auto& event : batch) {
            Dispatch(*event);
        }
        return batch.size();
    }

    // Drops queued events without delivering them. For shutdown, or after a
    // scene swap when the queue is full of events about things that no
    // longer exist.
    void Clear() {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.clear();
    }

    std::size_t GetQueuedCount() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queue.size();
    }

    // ---- configuration ------------------------------------------------

    void SetDeliverHandled(bool deliver) { deliverHandled = deliver; }
    bool GetDeliverHandled() const { return deliverHandled; }

    std::size_t GetListenerCount() const {
        std::size_t count = 0;
        for (const auto& [type, entries] : registry->listeners) {
            for (const auto& entry : entries) {
                if (entry.active) ++count;
            }
        }
        return count;
    }

    template<typename E>
    std::size_t GetListenerCount() const {
        auto it = registry->listeners.find(E::StaticTypeID());
        if (it == registry->listeners.end()) return 0;
        return static_cast<std::size_t>(
            std::count_if(it->second.begin(), it->second.end(),
                          [](const ListenerEntry& e) { return e.active; }));
    }

private:
    void Dispatch(Event& event) {
        Registry& reg = *registry;

        auto it = reg.listeners.find(event.GetTypeID());
        if (it == reg.listeners.end()) return;

        ++reg.dispatchDepth;
        {
            // By index, not iterator: Subscribe() during dispatch is parked
            // in pendingAdds so the vector cannot grow under us, but reading
            // size() each time keeps that assumption from being load-bearing.
            std::vector<ListenerEntry>& entries = it->second;
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (!entries[i].active) continue; // unsubscribed mid-dispatch

                if (entries[i].fn(event)) {
                    event.SetHandled();
                    if (!deliverHandled) break;
                }
            }
        }
        --reg.dispatchDepth;

        if (reg.dispatchDepth == 0) ApplyDeferred(reg);
    }

    static void InsertSorted(std::vector<ListenerEntry>& entries, ListenerEntry entry) {
        // Descending by priority; upper_bound keeps equal priorities in
        // subscription order.
        auto pos = std::upper_bound(entries.begin(), entries.end(), entry.priority,
            [](int priority, const ListenerEntry& e) { return priority > e.priority; });
        entries.insert(pos, std::move(entry));
    }

    static void ApplyDeferred(Registry& reg) {
        if (reg.needsCompaction) {
            for (auto& [type, entries] : reg.listeners) {
                std::erase_if(entries, [](const ListenerEntry& e) { return !e.active; });
            }
            reg.needsCompaction = false;
        }

        for (auto& [type, entry] : reg.pendingAdds) {
            InsertSorted(reg.listeners[type], std::move(entry));
        }
        reg.pendingAdds.clear();
    }

    static void Unsubscribe(Registry& reg, EventTypeID typeID, ListenerID id) {
        // The listener may still be parked in pendingAdds if it was
        // subscribed and dropped within the same dispatch.
        std::erase_if(reg.pendingAdds, [&](const auto& add) {
            return add.first == typeID && add.second.id == id;
        });

        auto it = reg.listeners.find(typeID);
        if (it == reg.listeners.end()) return;

        auto entry = std::find_if(it->second.begin(), it->second.end(),
            [&](const ListenerEntry& e) { return e.id == id; });
        if (entry == it->second.end()) return;

        if (reg.dispatchDepth > 0) {
            // Cannot erase while Dispatch() is indexing this vector, and
            // cannot destroy the callable either since it may be the one
            // running right now. Deactivate, compact after the walk.
            entry->active = false;
            reg.needsCompaction = true;
        } else {
            it->second.erase(entry);
        }
    }
};

inline void EventSubscription::Reset() {
    if (listenerID == 0) return;

    if (auto reg = registry.lock()) {
        EventBus::Unsubscribe(*reg, typeID, listenerID);
    }
    registry.reset();
    listenerID = 0;
}

} // namespace GNC
