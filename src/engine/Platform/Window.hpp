#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

struct GLFWwindow;

namespace GNC {

class EventBus;

// Window
// The GLFW window, without a graphics API context, and GLFW itself, so there
// is exactly one. Its callbacks only enqueue events on the bus, and detect the
// drags and double clicks GLFW does not report.
class Window {
public:
    // A second press within this many seconds (the Windows default) and
    // screen coordinates of the first is a double click.
    static constexpr double DoubleClickTime = 0.5;
    static constexpr double DoubleClickDistance = 4.0;

private:
    GLFWwindow* handle = nullptr;
    EventBus& eventBus;

    // For drag and double click detection.
    int heldButtons = 0;      // bit (1 << GLFW_MOUSE_BUTTON_*) per held button
    int lastPressButton = -1; // -1: the next press cannot complete a double click
    double lastPressTime = 0.0;
    double lastPressX = 0.0;
    double lastPressY = 0.0;

public:
    // The bus must outlive the window: the callbacks enqueue onto it.
    Window(int width, int height, const std::string& title, EventBus& bus);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void Close();

    void PollEvents();
    // Blocks until something happens (while minimized).
    void WaitEvents();

    // In pixels, not screen coordinates (they differ on high DPI). 0x0 while
    // minimized.
    vk::Extent2D GetFramebufferExtent() const;
    bool IsMinimized() const;

    // Skips the bus, so ImGui cannot swallow the key; listeners track
    // KeyPressed/KeyReleased instead.
    bool IsKeyDown(int key) const;

    // Hidden and locked, so movement is unbounded for mouse look; raw motion
    // where the platform has it.
    void SetCursorCaptured(bool captured);
    bool IsCursorCaptured() const;

    // For libraries that drive GLFW themselves (the ImGui backend).
    GLFWwindow* GetNativeHandle() const { return handle; }

    // Instance extensions the surface needs (VK_KHR_surface plus the platform's).
    static std::vector<const char*> GetRequiredInstanceExtensions();
    vk::raii::SurfaceKHR CreateSurface(const vk::raii::Instance& instance) const;

private:
    // A member so the callback lambdas can reach the private state above.
    void InstallCallbacks();
    void OnMouseButton(int button, int action, int mods);
};

} // namespace GNC
