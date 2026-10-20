#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace GNC {

class GeometryPass;

// Lighting pass for deferred rendering
// Reads the G-buffer back with one fullscreen triangle and shades every pixel
// with the Cook-Torrance BRDF, once per light (shaders/lighting_pass.slang).
// The lights themselves arrive through set 0, which the Renderer fills from the
// scene's LightComponents each frame.
//
// The result is linear HDR radiance in OutputFormat. Nothing here clamps,
// exposes or gamma corrects it; that is the post-process pass's job.
class LightingPass : public RenderPass {
public:
    static constexpr vk::Format OutputFormat = vk::Format::eR16G16B16A16Sfloat;

private:
    RenderTargetContext context;
    vk::Extent2D extent;
    GeometryPass* geometryPass;

    Attachment output;

    // Set 1: the G-buffer. Written once: the G-buffer images never change
    // (until resizing exists), and one set serves every frame in flight for the
    // same reason one G-buffer does.
    // The pool is declared before the set, so it outlives it.
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
