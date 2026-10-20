#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"

namespace GNC {

// ImGui pass
// Draws ImGui's main window over the post-processed swapchain image: whatever
// ImGui::Render() produced this frame (see UI/ImGuiLayer). Loads the image
// rather than clearing it, and leaves it as a color attachment like the passes
// before it.
//
// Owns no GPU objects. The pipeline, vertex buffers and font atlas belong to
// the ImGui Vulkan backend, which ImGuiLayer sets up. Without an ImGui context
// the pass draws nothing.
class ImGuiPass : public RenderPass {
private:
    vk::Extent2D extent;

public:
    ImGuiPass(const std::string& name, vk::Extent2D renderExtent);

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;
};

} // namespace GNC
