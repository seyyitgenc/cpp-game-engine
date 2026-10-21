#pragma once

#include <cstdint>
#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

class Renderer;

// Viewport panel
// The 3D view: the Renderer's scene image, which it sizes to the panel. The
// mouse over the image goes to the camera instead of ImGui. Docks into the
// central node the first time.
//
// Holding the left button on the image makes ImGui take the keyboard, so
// look around with the right one. Dragged out into an OS window it still
// shows the scene, but that window's input bypasses the event bus, so the
// camera does not respond there.
class ViewportPanel : public ImGuiPanel {
private:
    Renderer& renderer;

    // The scene image as registered with the ImGui Vulkan backend, and the
    // Renderer::GetSceneVersion() it was registered for.
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
