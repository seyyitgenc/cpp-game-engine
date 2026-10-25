#pragma once

#include <cstdint>
#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

class Renderer;

// Viewport panel
// The Renderer's scene image, sized to the panel. The mouse over it goes to
// the camera, not ImGui. In a dragged-out OS window it still shows the
// scene, but the camera gets no input there.
class ViewportPanel : public ImGuiPanel {
private:
    Renderer& renderer;

    // As registered with the Vulkan backend, for this scene version.
    vk::DescriptorSet texture;
    std::uint64_t textureVersion = 0;

public:
    // The renderer must outlive the panel.
    ViewportPanel(const std::string& name, Renderer& rend);
    // Needs the Vulkan backend, which ImGuiLayer shuts down after its panels.
    ~ViewportPanel() override;

    ViewportPanel(const ViewportPanel&) = delete;
    ViewportPanel& operator=(const ViewportPanel&) = delete;

    bool DocksInCentralNode() const override { return true; }
    void Render() override;

private:
    void UpdateTexture();
};

} // namespace GNC
