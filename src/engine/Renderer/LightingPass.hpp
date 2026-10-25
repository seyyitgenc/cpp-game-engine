#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace GNC {

class GeometryPass;

// Lighting pass for deferred rendering
// Shades the G-buffer with one fullscreen triangle, Cook-Torrance over every
// light in set 0, into linear HDR (OutputFormat). Exposure and tonemapping
// are the post-process pass's.
class LightingPass : public RenderPass {
public:
    static constexpr vk::Format OutputFormat = vk::Format::eR16G16B16A16Sfloat;

private:
    RenderTargetContext context;
    vk::Extent2D extent;
    GeometryPass* geometryPass;

    Attachment output;

    // Set 1: the G-buffer. Written once: a resize rebuilds the whole pass, and
    // one set serves every frame in flight, as the one G-buffer does. The
    // pool is declared first, so it outlives the set.
    vk::raii::DescriptorSetLayout gBufferSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    vk::raii::DescriptorSet gBufferSet = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;

public:
    LightingPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                 vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, GeometryPass* gPass);

    // Linear HDR scene color. In eShaderReadOnlyOptimal once the pass has run.
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
