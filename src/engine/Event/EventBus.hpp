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

// Behind a shared_ptr, so subscription tokens can hold a weak_ptr to it.
struct EventRegistry {
    using ListenerID = std::uint64_t;
    using ErasedListener = std::function<bool(Event&)>;

    struct ListenerEntry {
        ListenerID id;
        int priority;
        ErasedListener fn;

        // Cleared rather than erased when unsubscribed mid-dispatch: the
        // listener may be the one running, and destroying its closure would
        // free its captures under it.
        bool active = true;
    };

    std::unordered_map<EventTypeID, std::vector<ListenerEntry>> listeners;
    ListenerID nextListenerID = 1; // 0 means "no subscription"

    // While Flush() walks the listener vectors (dispatchDepth > 0),
    // additions wait in pendingAdds and removals only deactivate; both are
    // applied after the walk.
    int dispatchDepth = 0;
    std::vector<std::pair<EventTypeID, ListenerEntry>> pendingAdds;
    bool needsCompaction = false;
};

} // namespace detail

// Event subscription
// RAII token from EventBus::Subscribe(): destroying it removes the listener,
// so an owner that keeps it as a member never leaves a dangling callback. It
// holds the bus weakly, so outliving the bus is harmless.
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

    // Unsubscribes now rather than at destruction.
    void Reset();
};

// Event bus
// Queue-only publish/subscribe: Enqueue() delivers nothing; Flush(), called
// once per frame, delivers everything. So listeners run at one known point
// of the frame, never inside a GLFW callback or on another thread.
//
// Enqueue() is thread safe; Subscribe(), Unsubscribe() and Flush() are main
// thread only. Listeners run from highest priority to lowest, ties in
// subscription order. The first to return true handles the event and stops
// delivery, which is how ImGui swallows input before the camera sees it.
class EventBus {
private:
    using Registry = detail::EventRegistry;
    using ListenerID = Registry::ListenerID;
    using ListenerEntry = Registry::ListenerEntry;

    friend class EventSubscription;

    std::shared_ptr<Registry> registry = std::make_shared<Registry>();

    std::vector<std::unique_ptr<Event>> queue;
    std::mutex queueMutex;

    // Keep delivering after a listener handles the event, for a debugging
    // listener that observes everything.
    bool deliverHandled = false;

public:
    EventBus() = default;
    ~EventBus() = default;

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // -----------
    // Subscribing
    // -----------

    // For events of exactly type E. Keep the token: dropping it unsubscribes.
    template<typename E>
    [[nodiscard]] EventSubscription Subscribe(std::function<bool(E&)> listener, int priority = 0) {
        static_assert(std::is_base_of_v<Event, E>, "E must derive from GNC::Event");

        Registry& reg = *registry;
        const EventTypeID typeID = E::StaticTypeID();
        const ListenerID id = reg.nextListenerID++;

        // Safe: entries under E::StaticTypeID() are only invoked for events
        // whose GetTypeID() matches.
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

    template<typename E, typename F>
        requires std::is_invocable_r_v<bool, F&, E&>
    [[nodiscard]] EventSubscription Subscribe(F&& listener, int priority = 0) {
        return Subscribe<E>(std::function<bool(E&)>(std::forward<F>(listener)), priority);
    }

    // ----------
    // Publishing
    // ----------

    // Safe from any thread.
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

    // Replaces the most recent queued event of the same type instead of
    // adding another, for events where only the latest value matters (a
    // window resize fires dozens per frame).
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

    // ----------
    // Delivering
    // ----------

    // Main thread, once per frame. Events a listener enqueues land in the
    // next Flush(), so a frame's event count stays bounded.
    std::size_t Flush() {
        std::vector<std::unique_ptr<Event>> batch;
        {
            // Dispatch outside the lock, so a slow listener never stalls a
            // producer thread.
            std::lock_guard<std::mutex> lock(queueMutex);
            batch.swap(queue);
        }

        for (auto& event : batch) {
            Dispatch(*event);
        }
        return batch.size();
    }

    // Drops queued events undelivered.
    void Clear() {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.clear();
    }

    std::size_t GetQueuedCount() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queue.size();
    }

    // -------------
    // Configuration
    // -------------

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
            // By index, re-reading size(), so even an insertion here would
            // not invalidate the walk.
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
        // Descending priority; upper_bound keeps equal priorities in
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
        // Still parked here if subscribed and dropped within one dispatch.
        std::erase_if(reg.pendingAdds, [&](const auto& add) {
            return add.first == typeID && add.second.id == id;
        });

        auto it = reg.listeners.find(typeID);
        if (it == reg.listeners.end()) return;

        auto entry = std::find_if(it->second.begin(), it->second.end(),
            [&](const ListenerEntry& e) { return e.id == id; });
        if (entry == it->second.end()) return;

        if (reg.dispatchDepth > 0) {
            // Dispatch() is indexing this vector, and the callable may be
            // the one running: deactivate now, compact after the walk.
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
