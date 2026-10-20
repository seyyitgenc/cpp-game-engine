#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

struct GLFWwindow;

namespace GNC {

class EventBus;

// Window
// The GLFW window, created without a graphics API context (Vulkan brings its
// own). Owns GLFW itself too: initialized in the constructor, terminated in the
// destructor, so there is exactly one Window.
//
// Input reaches the rest of the engine as events (see Event/InputEvents.hpp):
// the GLFW callbacks enqueue them on the EventBus, which delivers them at its
// next Flush(). GLFW reports neither drags nor double clicks, so the callbacks
// detect those from the button and cursor events.
class Window {
public:
    // A second press of the same button within DoubleClickTime seconds (the
    // Windows default) and DoubleClickDistance screen coordinates of the
    // first is a double click.
    static constexpr double DoubleClickTime = 0.5;
    static constexpr double DoubleClickDistance = 4.0;

private:
    GLFWwindow* handle = nullptr;
    EventBus& eventBus;

    // Mouse state the callbacks keep for drag and double click detection.
    int heldButtons = 0;      // bit (1 << GLFW_MOUSE_BUTTON_*) per held button
    int lastPressButton = -1; // -1: the next press cannot complete a double click
    double lastPressTime = 0.0;
    double lastPressX = 0.0;
    double lastPressY = 0.0;

public:
    // The bus must outlive the window: the callbacks enqueue onto it.
    Window(int width, int height, const std::string& title, EventBus& bus);
    ~Window();

    // Owns the GLFW window (and GLFW itself), so exactly one of it exists.
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void Close();

    // Runs the GLFW callbacks, which enqueue this frame's events.
    void PollEvents();
    // Blocks until something happens, for when there is nothing to draw
    // (minimized).
    void WaitEvents();

    // In pixels, which is not the same as screen coordinates on high DPI
    // displays. 0x0 while minimized.
    vk::Extent2D GetFramebufferExtent() const;

    // GLFW key codes (GLFW_KEY_*). Skips the event bus, so a higher priority
    // listener cannot swallow the key first; listeners should track
    // KeyPressed/KeyReleased instead (see Event/InputEvents.hpp).
    bool IsKeyDown(int key) const;

    // Hides the cursor and locks it to the window, so mouse movement is
    // unbounded, as mouse look needs. Uses raw (unaccelerated) motion where
    // the platform has it.
    void SetCursorCaptured(bool captured);

    // Instance extensions the surface needs (VK_KHR_surface plus the platform's).
    static std::vector<const char*> GetRequiredInstanceExtensions();
    vk::raii::SurfaceKHR CreateSurface(const vk::raii::Instance& instance) const;

private:
    // A member so the callback lambdas can reach the private state above.
    void InstallCallbacks();
    void OnMouseButton(int button, int action, int mods);
};

} // namespace GNC
