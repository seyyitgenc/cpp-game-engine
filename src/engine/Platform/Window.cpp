#include "Window.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>

// Makes GLFW pull in vulkan.h and declare glfwCreateWindowSurface().
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Event/Events.hpp"
#include "Utils/Log.hpp"

namespace GNC {

namespace {

Window& WindowOf(GLFWwindow* window) {
    return *static_cast<Window*>(glfwGetWindowUserPointer(window));
}

} // namespace

Window::Window(int width, int height, const std::string& title, EventBus& bus) : eventBus(bus) {
    glfwSetErrorCallback([](int error, const char* description) {
        Log::writeFormatted(Log::Error, "GLFW error %d: %s", error, description);
    });

    if (!glfwInit()) {
        throw std::runtime_error("Window: failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!handle) {
        glfwTerminate();
        throw std::runtime_error("Window: failed to create the window");
    }

    // Window can be neither copied nor moved, so this pointer stays valid
    // for as long as the callbacks can run.
    glfwSetWindowUserPointer(handle, this);
    InstallCallbacks();
}

Window::~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(handle);
}

void Window::Close() {
    glfwSetWindowShouldClose(handle, GLFW_TRUE);
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::WaitEvents() {
    glfwWaitEvents();
}

vk::Extent2D Window::GetFramebufferExtent() const {
    int width = 0, height = 0;
    glfwGetFramebufferSize(handle, &width, &height);
    return { static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height) };
}

bool Window::IsKeyDown(int key) const {
    return glfwGetKey(handle, key) == GLFW_PRESS;
}

void Window::SetCursorCaptured(bool captured) {
    glfwSetInputMode(handle, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);
    }
}

std::vector<const char*> Window::GetRequiredInstanceExtensions() {
    std::uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (!extensions) {
        throw std::runtime_error("Window: GLFW found no Vulkan support on this system");
    }
    return { extensions, extensions + count };
}

vk::raii::SurfaceKHR Window::CreateSurface(const vk::raii::Instance& instance) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(*instance, handle, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Window: failed to create the window surface");
    }
    return vk::raii::SurfaceKHR(instance, surface);
}

// GLFW calls these from inside glfwPollEvents(). They only enqueue; every
// listener runs later, at the bus's Flush().
void Window::InstallCallbacks() {
    glfwSetFramebufferSizeCallback(handle, [](GLFWwindow* w, int width, int height) {
        // Coalesced: a drag fires dozens of these per frame and only the last
        // size matters.
        WindowOf(w).eventBus.EnqueueCoalesced<WindowResizeEvent>(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    });
    glfwSetWindowCloseCallback(handle, [](GLFWwindow* w) {
        WindowOf(w).eventBus.Enqueue<WindowCloseEvent>();
    });
    glfwSetWindowFocusCallback(handle, [](GLFWwindow* w, int focused) {
        Window& window = WindowOf(w);
        if (focused == GLFW_FALSE) {
            // A button released in another window never reaches this one, so
            // stop treating any as held rather than report drags forever.
            window.heldButtons = 0;
            window.lastPressButton = -1;
        }
        window.eventBus.Enqueue<WindowFocusEvent>(focused == GLFW_TRUE);
    });

    glfwSetKeyCallback(handle, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
        if (action == GLFW_RELEASE) {
            WindowOf(w).eventBus.Enqueue<KeyReleasedEvent>(key, scancode, mods);
        } else {
            WindowOf(w).eventBus.Enqueue<KeyPressedEvent>(key, scancode, mods, action == GLFW_REPEAT);
        }
    });
    glfwSetCharCallback(handle, [](GLFWwindow* w, unsigned int codepoint) {
        WindowOf(w).eventBus.Enqueue<CharTypedEvent>(codepoint);
    });

    glfwSetCursorPosCallback(handle, [](GLFWwindow* w, double x, double y) {
        Window& window = WindowOf(w);
        window.eventBus.Enqueue<MouseMovedEvent>(x, y);
        if (window.heldButtons != 0) {
            window.eventBus.Enqueue<MouseDraggedEvent>(x, y, window.heldButtons);
        }
    });
    glfwSetScrollCallback(handle, [](GLFWwindow* w, double dx, double dy) {
        WindowOf(w).eventBus.Enqueue<MouseScrolledEvent>(dx, dy);
    });
    glfwSetMouseButtonCallback(handle, [](GLFWwindow* w, int button, int action, int mods) {
        WindowOf(w).OnMouseButton(button, action, mods);
    });
}

void Window::OnMouseButton(int button, int action, int mods) {
    if (action == GLFW_RELEASE) {
        heldButtons &= ~(1 << button);
        eventBus.Enqueue<MouseButtonReleasedEvent>(button, mods);
        return;
    }

    heldButtons |= 1 << button;
    eventBus.Enqueue<MouseButtonPressedEvent>(button, mods);

    const double now = glfwGetTime();
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(handle, &x, &y);

    const bool doubleClick = button == lastPressButton
        && now - lastPressTime <= DoubleClickTime
        && std::abs(x - lastPressX) <= DoubleClickDistance
        && std::abs(y - lastPressY) <= DoubleClickDistance;

    if (doubleClick) {
        eventBus.Enqueue<MouseDoubleClickEvent>(button, mods);
        // A third press starts a new pair instead of making a second double
        // click out of presses two and three.
        lastPressButton = -1;
    } else {
        lastPressButton = button;
        lastPressTime = now;
        lastPressX = x;
        lastPressY = y;
    }
}

} // namespace GNC
