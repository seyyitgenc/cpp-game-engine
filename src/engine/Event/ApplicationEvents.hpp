#pragma once

#include "Event.hpp"

#include <cstdint>
#include <string>
#include <typeindex>
#include <utility>

namespace GNC {

// Publish with EnqueueCoalesced(): only the last size matters. Listeners
// should only mark the swapchain dirty; Flush() runs before the previous
// frame's fence is waited on.
class WindowResizeEvent : public Event {
private:
    std::uint32_t width;
    std::uint32_t height;

public:
    GNC_EVENT_TYPE(WindowResizeEvent, EventCategory::Application)

    WindowResizeEvent(std::uint32_t w, std::uint32_t h) : width(w), height(h) {}

    std::uint32_t GetWidth() const { return width; }
    std::uint32_t GetHeight() const { return height; }

    // Minimized windows report 0x0.
    bool IsMinimized() const { return width == 0 || height == 0; }
};

class WindowCloseEvent : public Event {
public:
    GNC_EVENT_TYPE(WindowCloseEvent, EventCategory::Application)
};

class WindowFocusEvent : public Event {
private:
    bool focused;

public:
    GNC_EVENT_TYPE(WindowFocusEvent, EventCategory::Application)

    explicit WindowFocusEvent(bool isFocused) : focused(isFocused) {}

    bool IsFocused() const { return focused; }
};

// Once per drawn frame, after that frame's input. deltaTime in seconds,
// capped at App::MaxDeltaTime.
class FrameEvent : public Event {
private:
    float deltaTime;

public:
    GNC_EVENT_TYPE(FrameEvent, EventCategory::Application)

    explicit FrameEvent(float dt) : deltaTime(dt) {}

    float GetDeltaTime() const { return deltaTime; }
};

// A request to quit (Esc, File > Quit); App closes the window.
// WindowCloseEvent is the close button, which GLFW acts on by itself.
class QuitApplicationEvent : public Event {
public:
    GNC_EVENT_TYPE(QuitApplicationEvent, EventCategory::Application)
};

// For anything holding a Vulkan object built from a resource (a pipeline
// from a shader) to rebuild it. Meant to be enqueued per resource
// HotReloadResourceManager rebuilds; nothing enqueues it yet.
class ResourceReloadedEvent : public Event {
private:
    std::type_index resourceType;
    std::string resourceId;

public:
    GNC_EVENT_TYPE(ResourceReloadedEvent, EventCategory::Resource)

    ResourceReloadedEvent(std::type_index type, std::string id)
        : resourceType(type), resourceId(std::move(id)) {}

    std::type_index GetResourceType() const { return resourceType; }
    const std::string& GetResourceId() const { return resourceId; }

    template<typename T>
    bool IsType() const { return resourceType == std::type_index(typeid(T)); }
};

} // namespace GNC
