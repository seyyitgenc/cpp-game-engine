#pragma once

#include "Event.hpp"

#include <cstdint>
#include <string>
#include <typeindex>
#include <utility>

namespace GNC {

// Fired by the framebuffer-size callback. Publish with EnqueueCoalesced():
// a drag produces many of these per frame and only the last size matters.
// The renderer's listener should mark the swapchain dirty and recreate it at
// the start of the next frame, never inside the listener itself, since Flush()
// runs before the previous frame's fence has necessarily been waited on.
class WindowResizeEvent : public Event {
private:
    std::uint32_t width;
    std::uint32_t height;

public:
    GNC_EVENT_TYPE(WindowResizeEvent, EventCategory::Application)

    WindowResizeEvent(std::uint32_t w, std::uint32_t h) : width(w), height(h) {}

    std::uint32_t GetWidth() const { return width; }
    std::uint32_t GetHeight() const { return height; }

    // Minimised windows report 0x0; there is nothing to render into.
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

// Fired from HotReloadResourceManager::ApplyPendingReloads() on the main
// thread, once per resource that was actually rebuilt. Anything that cached
// a Vulkan object derived from the resource (a pipeline built from a shader
// module, a descriptor set pointing at a texture view) listens for this and
// rebuilds its own copy.
//
// The resource is identified the same way the manager's table keys it, so a
// listener can compare against ResourceHandle::GetId() directly.
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
