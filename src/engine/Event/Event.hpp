#pragma once

#include <cstdint>
#include <type_traits>

namespace GNC {

using EventTypeID = std::uint32_t;

// A unique id per event type, assigned on first use, so the bus can key its
// listener table by integer.
class EventTypeIDSystem {
private:
    static inline EventTypeID nextTypeID = 0;

public:
    template<typename T>
    static EventTypeID GetTypeID() {
        static const EventTypeID typeID = nextTypeID++;
        return typeID;
    }
};

// Coarse flags on top of the exact type, to ask "is this input?" without
// listing every input event type.
enum class EventCategory : std::uint32_t {
    None        = 0,
    Application = 1u << 0, // window, lifecycle
    Input       = 1u << 1, // any device
    Keyboard    = 1u << 2,
    Mouse       = 1u << 3,
    MouseButton = 1u << 4,
    Gamepad     = 1u << 5,
    Resource    = 1u << 6, // asset (re)loads
    Scene       = 1u << 7, // entity lifecycle and the like
    User        = 1u << 8, // game-defined events, declared outside the engine
};

constexpr EventCategory operator|(EventCategory a, EventCategory b) {
    using U = std::underlying_type_t<EventCategory>;
    return static_cast<EventCategory>(static_cast<U>(a) | static_cast<U>(b));
}

constexpr EventCategory operator&(EventCategory a, EventCategory b) {
    using U = std::underlying_type_t<EventCategory>;
    return static_cast<EventCategory>(static_cast<U>(a) & static_cast<U>(b));
}

constexpr bool Any(EventCategory c) {
    return static_cast<std::underlying_type_t<EventCategory>>(c) != 0;
}

// Base event
// Events own their payload: they wait in the queue until Flush(), maybe
// enqueued from another thread, so nothing may point at the publisher's
// stack.
class Event {
private:
    bool handled = false;

public:
    virtual ~Event() = default;

    virtual EventTypeID GetTypeID() const = 0;
    virtual EventCategory GetCategories() const = 0;

    // For logging; static storage.
    virtual const char* GetName() const = 0;

    bool IsInCategory(EventCategory category) const {
        return Any(GetCategories() & category);
    }

    // Set once a listener returns true.
    bool IsHandled() const { return handled; }
    void SetHandled(bool isHandled = true) { handled = isHandled; }
};

} // namespace GNC

// Fills in the virtual boilerplate for a concrete event type:
//
//     class WindowResizeEvent : public Event {
//     public:
//         GNC_EVENT_TYPE(WindowResizeEvent, EventCategory::Application)
//     };
#define GNC_EVENT_TYPE(TypeName, Categories)                                      \
    static ::GNC::EventTypeID StaticTypeID() {                                    \
        return ::GNC::EventTypeIDSystem::GetTypeID<TypeName>();                   \
    }                                                                             \
    ::GNC::EventTypeID GetTypeID() const override { return StaticTypeID(); }      \
    ::GNC::EventCategory GetCategories() const override { return (Categories); }  \
    const char* GetName() const override { return #TypeName; }
