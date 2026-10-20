#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace GNC {

class LightingPass;

// Post-processing pass
// Exposure and tonemapping from the lighting pass's linear HDR output into the
// swapchain image (FrameContext::targetView). The first pass to write that
// image; it leaves it as a color attachment for the passes after it.
// shaders/post_process.slang.
class PostProcessPass : public RenderPass {
private:
    RenderTargetContext context;
    vk::Extent2D extent;
    LightingPass* lightingPass;

    // Without an sRGB swapchain format the shader applies the sRGB transfer
    // function itself.
    bool encodeSrgb;

    // Scales scene radiance before tonemapping. 1 suits the current scene;
    // physically based light units will want it driven by a camera exposure
    // model (EV100) later.
    float exposure = 1.0f;

    // Set 1: the lighting output. The pool is declared before the set, so it
    // outlives it.
    vk::raii::DescriptorSetLayout inputSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    vk::raii::DescriptorSet inputSet = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;

public:
    // targetFormat is the swapchain's; targetIsSrgb whether it encodes on write.
    PostProcessPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                    vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, LightingPass* lPass,
                    vk::Format targetFormat, bool targetIsSrgb);

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;

private:
    void CreateDescriptorSet();
    void CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, vk::Format targetFormat);
};

} // namespace GNC
