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
// Dear ImGui on its GLFW and Vulkan backends: owns the context, the backends,
// the panels and the menu bar. Input arrives through the bus at
// EventPriority, and is swallowed while ImGui wants that device. Per frame:
// Render(), Renderer::Render(), RenderPlatformWindows().
class ImGuiLayer {
public:
    // Above every other listener, so ImGui sees input first.
    static constexpr int EventPriority = 1000;

    // Lets panels leave the main window (ImGuiConfigFlags_ViewportsEnable).
    static constexpr bool EnableViewports = false;

    // ImGui_ImplVulkan_AddTexture images (the Viewport's), beyond the font
    // atlas.
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

    // What the backend's main-window pipeline was built for.
    vk::Format pipelineFormat = vk::Format::eUndefined;

    // In render order.
    std::vector<std::unique_ptr<ImGuiPanel>> panels;

    // After the panels, so it is destroyed first: its items point at them.
    ImGuiMenu menuBar;

    std::vector<EventSubscription> subscriptions;

public:
    // The window, Vulkan context and renderer must outlive the layer.
    ImGuiLayer(EventBus& bus, Window& win, const VulkanContext& vulkanContext, const Renderer& rend);

    // Waits for the GPU, then destroys the panels before the backends, so
    // they can still free their textures.
    ~ImGuiLayer();

    // There is one ImGui context, and the listeners capture this.
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    // ------
    // Panels
    // ------

    // Constructs T(name, args...). If the name is taken, returns that panel
    // when it is a T, and nullptr when it is not.
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

    ImGuiPanel* GetPanel(const std::string& name) const;

    // --------
    // Menu Bar
    // --------
    ImGuiMenu& GetMenuBar() { return menuBar; }

    // -----
    // Frame
    // -----

    // Leaves the draw data ImGuiPass records: call before Renderer::Render()
    // and after any Renderer::Resize() of the frame.
    void Render();

    // Draws and presents the panels outside the main window. After
    // Renderer::Render(), every frame Render() ran.
    void RenderPlatformWindows();

private:
    void LoadFonts();
    void NewFrame();
    void CreateMainPipeline(vk::Format format);
    void SubscribeToEvents(EventBus& bus);
};

} // namespace GNC
