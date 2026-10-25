#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace GNC {

class LightingPass;

// Post-processing pass
// Exposure and tonemapping from the lighting pass's linear HDR output into the
// scene image (GetOutput()), which the Viewport panel shows through ImGui.
// Leaves it in eShaderReadOnlyOptimal. shaders/post_process.slang.
class PostProcessPass : public RenderPass {
public:
    // UNORM, holding values the shader has already sRGB encoded: ImGui copies
    // texels to the UNORM swapchain as they are. An sRGB format would be
    // decoded when sampled and show too dark.
    static constexpr vk::Format OutputFormat = vk::Format::eR8G8B8A8Unorm;

private:
    RenderTargetContext context;
    vk::Extent2D extent;
    LightingPass* lightingPass;

    Attachment output;

    // Physical light units will want a camera exposure model (EV100) here.
    float exposure = 1.0f;

    // Set 1: the lighting output. The pool is declared before the set, so it
    // outlives it.
    vk::raii::DescriptorSetLayout inputSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    vk::raii::DescriptorSet inputSet = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;

public:
    PostProcessPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                    vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, LightingPass* lPass);

    const Attachment& GetOutput() const { return output; }

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;

private:
    void CreateDescriptorSet();
    void CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders);
};

} // namespace GNC
