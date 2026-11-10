#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "FrameData.hpp"
#include "RenderPass.hpp"
#include "RenderTarget.hpp"
#include "ShaderTypes.hpp"

namespace GNC {

class FrustumCulling;
class GeometryPass;
class TextureTable;

// Sprite pass
// Draws every visible SpriteComponent into the sprite layer (GetOutput()),
// which the post-process pass lays over the tonemapped scene, so sprites
// show their own colors. Back to front, by layer and then by depth, alpha
// blended; depth tested against the G-buffer, never written. For picking
// and the outlines, it also writes the G-buffer's entity ids, and each
// pixel's topmost sprite into GetDrawOrder(). shaders/sprite.slang.
class SpritePass : public RenderPass {
public:
    // Per frame; sprites past it are not drawn.
    static constexpr std::uint32_t MaxSprites = 16384;

    // UNORM, holding sRGB encoded values, as the scene image does, here
    // premultiplied by alpha: blending happens on the encoded values, as
    // image editors and ImGui do it.
    static constexpr vk::Format OutputFormat = vk::Format::eR8G8B8A8Unorm;
    // 32 bits, though 16 would hold it: slangc gives a Texture2D<uint> the
    // image format R32ui, which the view read through it must have.
    static constexpr vk::Format DrawOrderFormat = vk::Format::eR32Uint;

private:
    struct SpriteBuffer {
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        GpuSprite* mapped = nullptr;
        vk::raii::DescriptorSet set = nullptr;
    };

    struct Sprite {
        // The draw order: by layer, the farthest first, then by entity id,
        // so sprites that tie keep one order.
        int layer;
        float depth;
        std::uint64_t entity;

        std::uint32_t texture; // TextureTable slot
        GpuSprite gpu;
    };

    // A run of sprites with one texture: one draw.
    struct Batch {
        std::uint32_t texture;
        std::uint32_t first;
        std::uint32_t count;
    };

    RenderTargetContext context;
    vk::Extent2D extent;
    FrustumCulling* cullingSystem;
    const TextureTable& textureTable;
    GeometryPass* geometryPass;

    Attachment output;
    Attachment drawOrder;

    // The pool before the sets, which free themselves into it.
    vk::raii::DescriptorSetLayout spriteSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::array<SpriteBuffer, MaxFramesInFlight> spriteBuffers;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;

    // Rebuilt every frame; members only to keep their storage.
    std::vector<Sprite> sprites;
    std::vector<Batch> batches;
    bool warnedAboutSpriteLimit = false;

public:
    SpritePass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
               vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, FrustumCulling* culling,
               const TextureTable& textures, GeometryPass* gPass);

    // Both are cleared every frame, and in eShaderReadOnlyOptimal once the
    // pass has run.

    // Transparent where there is no sprite.
    const Attachment& GetOutput() const { return output; }
    // 1 + the place in the frame's draw order of the last sprite drawn on
    // each pixel; 0 where there is none.
    const Attachment& GetDrawOrder() const { return drawOrder; }

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;

private:
    void CreateSpriteBuffers();
    void CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders);
    // Fills sprites, in draw order.
    void CollectSprites();
};

} // namespace GNC
