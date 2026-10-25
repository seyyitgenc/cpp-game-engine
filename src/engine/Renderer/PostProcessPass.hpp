#pragma once

#include <cstdint>
#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace GNC {

class GeometryPass;
class LightingPass;

// Post-processing pass
// Exposure and tonemapping from the lighting pass's linear HDR output into the
// scene image (GetOutput()), which the Viewport panel shows through ImGui,
// then outlines from the G-buffer's entity ids and depth. Leaves it in
// eShaderReadOnlyOptimal. shaders/post_process.slang.
class PostProcessPass : public RenderPass {
public:
    // UNORM, holding values the shader has already sRGB encoded: ImGui copies
    // texels to the UNORM swapchain as they are. An sRGB format would be
    // decoded when sampled and show too dark.
    static constexpr vk::Format OutputFormat = vk::Format::eR8G8B8A8Unorm;

private:
    RenderTargetContext context;
    vk::Extent2D extent;
    GeometryPass* geometryPass;
    LightingPass* lightingPass;

    Attachment output;

    // Physical light units will want a camera exposure model (EV100) here.
    float exposure = 1.0f;

public:
    // GeometryPushConstants::entityId values in [begin, end).
    struct IdRange {
        std::uint32_t begin = 0;
        std::uint32_t end = 0;
    };

private:
    IdRange selectedIds;
    IdRange hoveredIds;

    // Set 1: the lighting output, the entity ids and depth. The pool is
    // declared before the set, so it outlives it.
    vk::raii::DescriptorSetLayout inputSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    vk::raii::DescriptorSet inputSet = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;

public:
    PostProcessPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                    vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders,
                    GeometryPass* gPass, LightingPass* lPass);

    const Attachment& GetOutput() const { return output; }

    // This frame's; either may be empty.
    void SetOutlinedIds(IdRange selected, IdRange hovered) {
        selectedIds = selected;
        hoveredIds = hovered;
    }

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;

private:
    void CreateDescriptorSet();
    void CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders);
};

} // namespace GNC
