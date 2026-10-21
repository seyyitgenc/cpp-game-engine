#include "ViewportPanel.hpp"

#include <algorithm>
#include <cstdint>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "Renderer/Renderer.hpp"

namespace GNC {

ViewportPanel::ViewportPanel(const std::string& name, Renderer& rend) : ImGuiPanel(name), renderer(rend) {}

ViewportPanel::~ViewportPanel() {
    if (texture) ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
}

void ViewportPanel::Render() {
    // No padding: the image goes edge to edge.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool open = ImGui::Begin(GetName().c_str(), GetVisibleFlag(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (open) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const ImVec2 size(std::max(available.x, 1.0f), std::max(available.y, 1.0f));
        const ImVec2 scale = ImGui::GetWindowViewport()->FramebufferScale;

        // May rebuild the scene image, so before UpdateTexture().
        renderer.SetSceneExtent({
            static_cast<std::uint32_t>(size.x * scale.x),
            static_cast<std::uint32_t>(size.y * scale.y)
        });
        UpdateTexture();

        // The button takes left clicks, so dragging over the image does not
        // move a floating viewport. The right button is left to the camera.
        const ImVec2 min = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##scene", size);
        const ImTextureID textureId = reinterpret_cast<std::uintptr_t>(static_cast<VkDescriptorSet>(texture));
        ImGui::GetWindowDrawList()->AddImage(textureId, min, ImVec2(min.x + size.x, min.y + size.y));

        // Hovering it would make ImGui want the mouse, and swallow it.
        if (ImGui::IsItemHovered()) ImGui::SetNextFrameWantCaptureMouse(false);
    }
    ImGui::End();
}

// Every new scene version follows a wait for the GPU, so nothing in flight
// still reads the old descriptor set.
void ViewportPanel::UpdateTexture() {
    if (texture && textureVersion == renderer.GetSceneVersion()) return;

    if (texture) ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
    texture = ImGui_ImplVulkan_AddTexture(static_cast<VkImageView>(renderer.GetSceneView()), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    textureVersion = renderer.GetSceneVersion();
}

} // namespace GNC
