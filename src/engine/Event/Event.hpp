#pragma once

#include <cstdint>
#include <type_traits>

namespace GNC {

using EventTypeID = std::uint32_t;

// Event type identification
// Same idea as ComponentTypeIDSystem: every event type gets a unique id the
// first time it is asked for, so the bus can key its listener table by a plain
// integer instead of a string compare or a dynamic_cast per delivery.
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

// Event categories
// Coarse bit flags an event belongs to, on top of its exact type. A listener
// subscribes by exact type; categories exist for the one question type alone
// cannot answer cheaply: "is this something the UI layer wants to swallow?"
// An ImGui layer with keyboard focus consumes everything tagged Input and
// nothing else, without having to list every input event type by name.
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
// Events are plain value objects that *own* their payload. They sit in the
// bus's queue until the next Flush(), possibly having been enqueued from
// another thread, so nothing in here may point back at the publisher's stack.
class Event {
private:
    bool handled = false;

public:
    virtual ~Event() = default;

    virtual EventTypeID GetTypeID() const = 0;
    virtual EventCategory GetCategories() const = 0;

    // For logging. Static storage, never freed.
    virtual const char* GetName() const = 0;

    bool IsInCategory(EventCategory category) const {
        return Any(GetCategories() & category);
    }

    // Set by the bus once a listener returns true; later listeners for the
    // same event are not called. Exposed so a listener can also inspect it
    // when the bus is asked to keep delivering handled events.
    bool IsHandled() const { return handled; }
    void SetHandled(bool isHandled = true) { handled = isHandled; }
};

} // namespace GNC

// Fills in the virtual boilerplate for a concrete event type.
//
//     class WindowResizeEvent : public Event {
//     public:
//         GNC_EVENT_TYPE(WindowResizeEvent, EventCategory::Application)
//         ...
//     };
//
// StaticTypeID() is what the bus uses at subscribe time; GetTypeID() is the
// same value reached through the base pointer at dispatch time.
#define GNC_EVENT_TYPE(TypeName, Categories)                                      \
    static ::GNC::EventTypeID StaticTypeID() {                                    \
        return ::GNC::EventTypeIDSystem::GetTypeID<TypeName>();                   \
    }                                                                             \
    ::GNC::EventTypeID GetTypeID() const override { return StaticTypeID(); }      \
    ::GNC::EventCategory GetCategories() const override { return (Categories); }  \
    const char* GetName() const override { return #TypeName; }
