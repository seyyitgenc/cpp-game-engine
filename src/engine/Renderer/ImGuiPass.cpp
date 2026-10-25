#include "ImGuiPass.hpp"

#include <array>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "Utils/VulkanHelpers.hpp"

namespace GNC {

ImGuiPass::ImGuiPass(const std::string& name, vk::Extent2D renderExtent) : RenderPass(name), extent(renderExtent) {}

void ImGuiPass::BeginPass(const FrameContext& frame) {
    // The submit waits for acquire at eColorAttachmentOutput; starting the src
    // scope there puts the layout transition after that wait. The old
    // contents are not needed.
    const vk::ImageMemoryBarrier2 barrier = makeImageBarrier(frame.targetImage, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite);
    frame.commandBuffer.pipelineBarrier2(vk::DependencyInfo {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });

    // Only the UI goes here; the scene arrives as the Viewport's texture.
    const vk::RenderingAttachmentInfo colorInfo {
        .imageView = frame.targetView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearColorValue(std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f })
    };
    frame.commandBuffer.beginRendering(vk::RenderingInfo {
        .renderArea = { { 0, 0 }, extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorInfo
    });
}

void ImGuiPass::Render(const FrameContext& frame) {
    if (!ImGui::GetCurrentContext()) return;

    // Null until ImGui::Render() has run for this frame.
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData) return;

    // Sets its own pipeline, viewport and scissor. Font atlas uploads go
    // through the backend's own command buffer and submit, not this one.
    ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(*frame.commandBuffer));
}

void ImGuiPass::EndPass(const FrameContext& frame) {
    frame.commandBuffer.endRendering();
}

} // namespace GNC
