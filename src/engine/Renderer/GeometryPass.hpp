#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "FrameData.hpp"
#include "RenderPass.hpp"
#include "RenderTarget.hpp"
#include "ShaderTypes.hpp"

namespace GNC {

class FrustumCulling;
class Mesh;
class TextureTable;
struct Material;

// G-buffer
// Laid out as in shaders/common/gbuffer.slang. One, shared by all frames in
// flight: GeometryPass's first barrier waits for the previous frame's
// lighting pass to finish reading it.
struct GBuffer {
    static constexpr vk::Format BaseColorFormat = vk::Format::eR8G8B8A8Srgb;
    static constexpr vk::Format NormalFormat = vk::Format::eR16G16Sfloat;
    static constexpr vk::Format MaterialFormat = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format EmissiveFormat = vk::Format::eR16G16B16A16Sfloat;
    static constexpr vk::Format EntityIdFormat = vk::Format::eR32Uint;
    static constexpr vk::Format DepthFormat = vk::Format::eD32Sfloat;

    // Same order as the SV_Target outputs in shaders/geometry_pass.slang.
    static constexpr std::uint32_t ColorCount = 5;
    static constexpr std::array<vk::Format, ColorCount> ColorFormats {
        BaseColorFormat, NormalFormat, MaterialFormat, EmissiveFormat, EntityIdFormat
    };
    static constexpr std::uint32_t EntityIdTarget = 4;

    Attachment baseColor;
    Attachment normal;
    Attachment material;
    Attachment emissive;
    // Read by the post-process pass for outlines and copied out by
    // Renderer for picking; lighting never reads it.
    Attachment entityId;
    Attachment depth;

    // In ColorFormats order.
    std::array<const Attachment*, ColorCount> ColorAttachments() const {
        return { &baseColor, &normal, &material, &emissive, &entityId };
    }
};

// Geometry pass for deferred rendering
// Draws every entity that survived culling into the G-buffer, unlit. Each
// frame it writes the materials it draws with into that frame's material
// buffer (set 2); their textures come from the TextureTable (set 1).
class GeometryPass : public RenderPass {
public:
    // Per frame; materials past it draw with the defaults.
    static constexpr std::uint32_t MaxMaterials = 4096;

private:
    struct MaterialBuffer {
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        GpuMaterial* mapped = nullptr;
        vk::raii::DescriptorSet set = nullptr;
    };

    // One per submesh.
    struct Draw {
        const Mesh* mesh;
        std::uint32_t firstIndex;
        std::uint32_t indexCount;
        glm::mat4 model;
        std::uint32_t materialIndex;
        std::uint32_t entityId;
        bool masked;
        bool doubleSided;
    };

    RenderTargetContext context;
    vk::Extent2D extent;
    FrustumCulling* cullingSystem;
    const TextureTable& textureTable;

    GBuffer gBuffer;

    // The pool before the sets, which free themselves into it.
    vk::raii::DescriptorSetLayout materialSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::array<MaterialBuffer, MaxFramesInFlight> materialBuffers;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline opaquePipeline = nullptr;
    // Discards below the alpha cutoff (ALPHA_MASK in the shader).
    vk::raii::Pipeline maskedPipeline = nullptr;

    // Rebuilt every frame; members only to keep their storage.
    std::unordered_map<const Material*, std::uint32_t> materialIndices;
    std::vector<Draw> draws;
    bool warnedAboutMaterialLimit = false;

public:
    GeometryPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                 vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, FrustumCulling* culling,
                 const TextureTable& textures);
    ~GeometryPass() override;

    const GBuffer& GetGBuffer() const { return gBuffer; }
    vk::Extent2D GetExtent() const { return extent; }

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;

private:
    void CreateGBuffer();
    void CreateMaterialBuffers();
    void CreatePipelines(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders);
    std::uint32_t AddMaterial(const Material* material, std::uint32_t& materialCount, GpuMaterial* gpuMaterials);
};

} // namespace GNC
