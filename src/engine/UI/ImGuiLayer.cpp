#include "ImGuiLayer.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Event/Events.hpp"
#include "Platform/Window.hpp"
#include "Renderer/FrameData.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Utils/FileSystem.hpp"
#include "Utils/Log.hpp"

namespace GNC {

namespace {

// The backend reports Vulkan results through this rather than throwing, and
// calls it for successes too.
void CheckVkResult(VkResult result) {
    if (result == VK_SUCCESS) return;

    const std::string message = vk::to_string(static_cast<vk::Result>(result));
    if (result < 0) {
        Log::write(Log::Error, RED_TEXT("[ImGui] "), "Vulkan error: ", message, "\n");
    } else {
        Log::write(Log::Warning, YELLOW_TEXT("[ImGui] "), "Vulkan: ", message, "\n");
    }
}

} // namespace

ImGuiLayer::ImGuiLayer(EventBus& bus, Window& win, const VulkanContext& vulkanContext, const Renderer& rend)
    : window(win), vulkan(vulkanContext), renderer(rend), iniPath(FileSystem::getPath("imgui.ini").string()) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    // Next to the executable, like every other resource, rather than in
    // whatever the working directory is.
    io.IniFilename = iniPath.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if constexpr (EnableViewports) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    // -----
    // Style
    // -----
    // Scaled for the primary monitor's DPI and rescaled as windows move
    // between monitors, as the backends' GLFW + Vulkan example does.
    const float dpiScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(dpiScale);
    style.FontScaleDpi = dpiScale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    // A panel dragged out of the main window becomes an OS window. Square
    // and opaque, it looks the same outside as it did inside.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // --------
    // Backends
    // --------
    // install_callbacks = false: input arrives through the bus instead, see
    // SubscribeToEvents().
    ImGui_ImplGlfw_InitForVulkan(window.GetNativeHandle(), false);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = vk::ApiVersion13; // what VulkanContext creates the instance for
    initInfo.Instance = static_cast<VkInstance>(*vulkan.GetInstance());
    initInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(*vulkan.GetPhysicalDevice());
    initInfo.Device = static_cast<VkDevice>(*vulkan.GetDevice());
    initInfo.QueueFamily = vulkan.GetQueueFamilyIndex();
    initInfo.Queue = static_cast<VkQueue>(*vulkan.GetQueue());
    // The backend makes its own descriptor pool, sized for the font atlas.
    initInfo.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
    // ImageCount is how many copies of its vertex and index buffers the
    // backend cycles through, and how many frames it waits before freeing a
    // texture: one per frame in flight. MinImageCount is only for the
    // swapchains of the windows it creates.
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = std::max<std::uint32_t>(MaxFramesInFlight, initInfo.MinImageCount);
    initInfo.UseDynamicRendering = true;
    initInfo.CheckVkResultFn = &CheckVkResult;
    ImGui_ImplVulkan_Init(&initInfo);

    CreateMainPipeline(renderer.GetTargetFormat());

    SubscribeToEvents(bus);
}

ImGuiLayer::~ImGuiLayer() {
    subscriptions.clear();

    vulkan.GetDevice().waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    // Also saves the layout to imgui.ini.
    ImGui::DestroyContext();
}

ImGuiPanel* ImGuiLayer::GetPanel(const std::string& name) const {
    const auto it = std::ranges::find_if(panels, [&](const std::unique_ptr<ImGuiPanel>& panel) {
        return panel->GetName() == name;
    });
    return it != panels.end() ? it->get() : nullptr;
}

void ImGuiLayer::Render() {
    NewFrame();

    // Panels dock to the window's edges. Passthru keeps the middle (the
    // central node) see-through and lets clicks there fall through to the
    // scene, so docked panels frame the 3D view instead of covering it.
    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    for (const std::unique_ptr<ImGuiPanel>& panel : panels) {
        panel->Render();
    }

    ImGui::Render();
}

void ImGuiLayer::RenderPlatformWindows() {
    if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) return;

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

void ImGuiLayer::NewFrame() {
    // Only when the format really changes, which a resize practically never
    // does. Rebuilding the pipeline after the backend has created a platform
    // window leaves the backend's own pipeline info for those windows
    // pointing at freed memory (imgui_impl_vulkan, 1.93.1 WIP). Renderer::
    // Resize() waited for the GPU, so the old pipeline is no longer in use.
    if (renderer.GetTargetFormat() != pipelineFormat) {
        CreateMainPipeline(renderer.GetTargetFormat());
    }

    // While the camera has the cursor captured, its position is unbounded
    // and hidden, and means nothing to ImGui. The backend leaves ignoring it
    // to the application.
    ImGuiIO& io = ImGui::GetIO();
    if (window.IsCursorCaptured()) {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// For the main window only; the backend builds the pipeline for the windows it
// creates itself, in the format it picks for their swapchains.
void ImGuiLayer::CreateMainPipeline(vk::Format format) {
    const vk::PipelineRenderingCreateInfo renderingInfo {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format
    };
    ImGui_ImplVulkan_PipelineInfo pipelineInfo = {};
    pipelineInfo.PipelineRenderingCreateInfo = renderingInfo;
    ImGui_ImplVulkan_CreateMainPipeline(&pipelineInfo);

    pipelineFormat = format;
}

// Each listener hands the event to the GLFW backend's matching callback,
// which translates keys, tracks modifiers, and turns window coordinates into
// screen ones when viewports are on. Returning true swallows the event.
void ImGuiLayer::SubscribeToEvents(EventBus& bus) {
    GLFWwindow* handle = window.GetNativeHandle();
    auto wantsMouse = [this] { return !window.IsCursorCaptured() && ImGui::GetIO().WantCaptureMouse; };

    // --------
    // Keyboard
    // --------
    subscriptions.push_back(bus.Subscribe<KeyPressedEvent>([handle](KeyPressedEvent& e) {
        // The backend ignores repeats: ImGui repeats a held key itself.
        ImGui_ImplGlfw_KeyCallback(handle, e.GetKey(), e.GetScancode(), e.IsRepeat() ? GLFW_REPEAT : GLFW_PRESS, e.GetMods());
        return ImGui::GetIO().WantCaptureKeyboard;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<KeyReleasedEvent>([handle](KeyReleasedEvent& e) {
        ImGui_ImplGlfw_KeyCallback(handle, e.GetKey(), e.GetScancode(), GLFW_RELEASE, e.GetMods());
        return false;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<CharTypedEvent>([handle](CharTypedEvent& e) {
        ImGui_ImplGlfw_CharCallback(handle, e.GetCodepoint());
        return ImGui::GetIO().WantCaptureKeyboard;
    }, EventPriority));

    // -----
    // Mouse
    // -----
    // While the cursor is captured, positions, presses and scrolling are left
    // to the camera. Releases still go to ImGui: it may have seen the press.
    subscriptions.push_back(bus.Subscribe<MouseMovedEvent>([this, handle, wantsMouse](MouseMovedEvent& e) {
        if (window.IsCursorCaptured()) return false;
        ImGui_ImplGlfw_CursorPosCallback(handle, e.GetX(), e.GetY());
        return wantsMouse();
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseButtonPressedEvent>([this, handle, wantsMouse](MouseButtonPressedEvent& e) {
        if (window.IsCursorCaptured()) return false;
        ImGui_ImplGlfw_MouseButtonCallback(handle, e.GetButton(), GLFW_PRESS, e.GetMods());
        return wantsMouse();
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseButtonReleasedEvent>([handle](MouseButtonReleasedEvent& e) {
        ImGui_ImplGlfw_MouseButtonCallback(handle, e.GetButton(), GLFW_RELEASE, e.GetMods());
        return false;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseScrolledEvent>([this, handle, wantsMouse](MouseScrolledEvent& e) {
        if (window.IsCursorCaptured()) return false;
        ImGui_ImplGlfw_ScrollCallback(handle, e.GetOffsetX(), e.GetOffsetY());
        return wantsMouse();
    }, EventPriority));

    // ImGui finds drags and double clicks itself, from the moves and presses
    // above; these only need hiding from the scene.
    subscriptions.push_back(bus.Subscribe<MouseDraggedEvent>([wantsMouse](MouseDraggedEvent&) {
        return wantsMouse();
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseDoubleClickEvent>([wantsMouse](MouseDoubleClickEvent&) {
        return wantsMouse();
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseEnterEvent>([handle](MouseEnterEvent&) {
        ImGui_ImplGlfw_CursorEnterCallback(handle, GLFW_TRUE);
        return false;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseLeaveEvent>([handle](MouseLeaveEvent&) {
        ImGui_ImplGlfw_CursorEnterCallback(handle, GLFW_FALSE);
        return false;
    }, EventPriority));

    // ------
    // Window
    // ------
    subscriptions.push_back(bus.Subscribe<WindowFocusEvent>([handle](WindowFocusEvent& e) {
        ImGui_ImplGlfw_WindowFocusCallback(handle, e.IsFocused() ? GLFW_TRUE : GLFW_FALSE);
        return false;
    }, EventPriority));
}

} // namespace GNC
