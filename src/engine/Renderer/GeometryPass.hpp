#pragma once

#include <array>
#include <cstdint>
#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace GNC {

class FrustumCulling;

// G-buffer
// Laid out as in shaders/common/gbuffer.slang. One, shared by all frames in
// flight: GeometryPass's first barrier waits for the previous frame's
// lighting pass to finish reading it.
struct GBuffer {
    static constexpr vk::Format BaseColorFormat = vk::Format::eR8G8B8A8Srgb;
    static constexpr vk::Format NormalFormat = vk::Format::eR16G16Sfloat;
    static constexpr vk::Format MaterialFormat = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format EmissiveFormat = vk::Format::eR16G16B16A16Sfloat;
    static constexpr vk::Format DepthFormat = vk::Format::eD32Sfloat;

    // Same order as the SV_Target outputs in shaders/geometry_pass.slang.
    static constexpr std::uint32_t ColorCount = 4;
    static constexpr std::array<vk::Format, ColorCount> ColorFormats {
        BaseColorFormat, NormalFormat, MaterialFormat, EmissiveFormat
    };

    Attachment baseColor;
    Attachment normal;
    Attachment material;
    Attachment emissive;
    Attachment depth;

    // In ColorFormats order.
    std::array<const Attachment*, ColorCount> ColorAttachments() const {
        return { &baseColor, &normal, &material, &emissive };
    }
};

// Geometry pass for deferred rendering
// Draws every entity that survived culling into the G-buffer, unlit.
class GeometryPass : public RenderPass {
private:
    RenderTargetContext context;
    vk::Extent2D extent;
    FrustumCulling* cullingSystem;

    GBuffer gBuffer;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;

public:
    GeometryPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                 vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, FrustumCulling* culling);
    ~GeometryPass() override;

    const GBuffer& GetGBuffer() const { return gBuffer; }
    vk::Extent2D GetExtent() const { return extent; }

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;

private:
    void CreateGBuffer();
    void CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders);
};

} // namespace GNC
