#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Event/EventBus.hpp"
#include "ImGuiMenu.hpp"
#include "ImGuiPanel.hpp"

namespace GNC {

class Renderer;
class VulkanContext;
class Window;

// ImGui layer
// Dear ImGui (docking branch, src/external/ImGui) on its official GLFW and
// Vulkan backends, and the manager of the UI's panels. Owns the ImGui context,
// both backends, every panel and the main menu bar. ImGuiPass, in the
// Renderer, records the main window's UI into each frame.
//
// The menu bar runs along the top of the main window, above the dockspace.
// Its menus (GetMenuBar(), an ImGuiMenu tree) hold commands, submenus and
// check mark items that show and hide panels; only visible panels render.
//
// Input reaches ImGui through the event bus, not through GLFW callbacks of its
// own: Window's callbacks stay the only ones on the main window. The listeners
// run at EventPriority, above everything else, hand each event to the GLFW
// backend, and swallow it while ImGui wants that device (io.WantCaptureMouse /
// io.WantCaptureKeyboard, as of the last frame). So a click on a panel never
// reaches the camera, and typing into a text field never moves it. Releases
// are never swallowed, and while the camera has the cursor captured the mouse
// is the camera's alone.
//
// With viewports enabled, a panel dragged out of the main window becomes an
// OS window with its own swapchain, which the backends create, feed input to
// and present by themselves in RenderPlatformWindows().
//
// Per frame, in the render traversal: Render(), Renderer::Render(),
// RenderPlatformWindows().
class ImGuiLayer {
public:
    // Above every other listener, so ImGui sees input first.
    static constexpr int EventPriority = 1000;

    // Lets panels leave the main window (ImGuiConfigFlags_ViewportsEnable).
    static constexpr bool EnableViewports = true;

    // Images panels show through ImGui_ImplVulkan_AddTexture (the Viewport's
    // scene image), on top of what the font atlas needs.
    static constexpr std::uint32_t MaxUserTextures = 8;

    // At 100% scale. Icons fill the whole em, so at the text's size they
    // would look too big.
    static constexpr float TextFontSize = 13.0f;
    static constexpr float IconFontSize = 10.0f;

private:
    Window& window;
    const VulkanContext& vulkan;
    const Renderer& renderer;

    // ImGui keeps only the pointer (io.IniFilename), so the string lives here.
    std::string iniPath;

    // The format the backend's pipeline for the main window was built for.
    vk::Format pipelineFormat = vk::Format::eUndefined;

    // In the order they were added, which is the order they render in.
    std::vector<std::unique_ptr<ImGuiPanel>> panels;

    // After the panels, so it is destroyed first: its items point at them.
    ImGuiMenu menuBar;

    std::vector<EventSubscription> subscriptions;

public:
    // The window, Vulkan context and renderer must outlive the layer. The
    // renderer is read for its swapchain format.
    ImGuiLayer(EventBus& bus, Window& win, const VulkanContext& vulkanContext, const Renderer& rend);

    // Waits for the GPU first: frames in flight may still read the backend's
    // buffers and font atlas. Destroys the panels before the backends, so
    // they can still free their textures.
    ~ImGuiLayer();

    // There is one ImGui context, and the listeners capture this.
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    // ------
    // Panels
    // ------

    // Constructs a T(name, args...) and adds it after the panels already
    // there. If a panel with that name exists, returns it instead when it is
    // a T, and nullptr when it is not, as RenderPassManager::AddRenderPass
    // does.
    template<typename T, typename... Args>
    T* AddPanel(const std::string& name, Args&&... args) {
        static_assert(std::is_base_of_v<ImGuiPanel, T>, "T must derive from ImGuiPanel");

        if (ImGuiPanel* existing = GetPanel(name)) {
            return dynamic_cast<T*>(existing);
        }

        auto panel = std::make_unique<T>(name, std::forward<Args>(args)...);
        T* panelPtr = panel.get();
        panels.push_back(std::move(panel));
        return panelPtr;
    }

    // nullptr when there is none.
    ImGuiPanel* GetPanel(const std::string& name) const;

    // --------
    // Menu Bar
    // --------
    ImGuiMenu& GetMenuBar() { return menuBar; }

    // -----
    // Frame
    // -----

    // Builds this frame's UI: the menu bar, a dockspace
    // then every visible panel. Leaves the draw data ImGuiPass
    // records, so call it before Renderer::Render(), and after any
    // Renderer::Resize() of the frame.
    void Render();

    // Draws and presents the panels outside the main window. Call after
    // Renderer::Render(), every frame Render() ran.
    void RenderPlatformWindows();

private:
    void LoadFonts();
    void NewFrame();
    void CreateMainPipeline(vk::Format format);
    void SubscribeToEvents(EventBus& bus);
};

} // namespace GNC
