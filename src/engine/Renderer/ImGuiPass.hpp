#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"

namespace GNC {

// ImGui pass
// The only pass that writes the swapchain image: clears it and draws this
// frame's ImGui draw data. Owns no GPU objects; the ImGui Vulkan backend does.
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
