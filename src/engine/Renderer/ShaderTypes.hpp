#pragma once

#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>

namespace GNC {

// C++ mirrors of the shader structs named above each one; the static_asserts
// pin them to the offsets the shader compiler reports. Every glm::vec3 is
// followed by a 4 byte scalar, so std140 and C++ agree without padding.

// shaders/common/frame.slang: MAX_LIGHTS
inline constexpr std::uint32_t MaxLights = 64;

// shaders/common/frame.slang: GpuLight
// Built from a LightComponent by Renderer::MakeGpuLight().
struct GpuLight {
    glm::vec3 position {0.0f};
    std::uint32_t type = 0; // LightType
    glm::vec3 direction {0.0f, 0.0f, -1.0f};
    float range = 0.0f;
    glm::vec3 color {1.0f};
    float intensity = 0.0f;
    float spotAngleScale = 0.0f;
    float spotAngleOffset = 1.0f;
    float pad[2] {};
};

// shaders/common/frame.slang: LightsData (set 0, binding 1)
struct LightsUniform {
    glm::vec3 ambientColor {0.0f};
    std::uint32_t lightCount = 0;
    GpuLight lights[MaxLights];
};

// shaders/geometry_pass.slang: DrawData (push constants)
struct GeometryPushConstants {
    glm::mat4 model {1.0f};
    // Into this frame's GpuMaterial buffer.
    std::uint32_t materialIndex = 0;
    // 1 + the index into the frame's visible entities; 0 is the background.
    std::uint32_t entityId = 0;
    std::uint32_t pad[2] {};
};

// shaders/geometry_pass.slang: GpuMaterial (set 2, binding 0, std430)
// Built from a Material by GeometryPass. Textures are TextureTable slots.
struct GpuMaterial {
    glm::vec4 baseColorFactor {1.0f};
    glm::vec3 emissive {0.0f}; // emissiveFactor * emissiveStrength
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    float alphaCutoff = 0.5f;
    std::uint32_t baseColorTexture = 0;
    std::uint32_t metallicRoughnessTexture = 0;
    std::uint32_t normalTexture = 0;
    std::uint32_t occlusionTexture = 0;
    std::uint32_t emissiveTexture = 0;
    std::uint32_t pad[3] {};
};

// shaders/post_process.slang: PostData (push constants)
struct PostProcessPushConstants {
    float exposure = 1.0f;
    std::uint32_t encodeSrgb = 0;
    // GeometryPushConstants::entityId ranges to outline, [begin, end).
    std::uint32_t selectedBegin = 0;
    std::uint32_t selectedEnd = 0;
    std::uint32_t hoveredBegin = 0;
    std::uint32_t hoveredEnd = 0;
};

// shaders/debug_draw.slang: DrawData (push constants)
struct DebugDrawPushConstants {
    glm::mat4 model {1.0f};
    // sRGB encoded, as the scene image holds it.
    glm::vec4 color {1.0f};
};

static_assert(offsetof(GpuLight, type) == 12);
static_assert(offsetof(GpuLight, direction) == 16);
static_assert(offsetof(GpuLight, range) == 28);
static_assert(offsetof(GpuLight, color) == 32);
static_assert(offsetof(GpuLight, intensity) == 44);
static_assert(offsetof(GpuLight, spotAngleScale) == 48);
static_assert(offsetof(GpuLight, spotAngleOffset) == 52);
static_assert(sizeof(GpuLight) == 64);

static_assert(offsetof(LightsUniform, lightCount) == 12);
static_assert(offsetof(LightsUniform, lights) == 16);
static_assert(sizeof(LightsUniform) == 16 + sizeof(GpuLight) * MaxLights);

static_assert(offsetof(GeometryPushConstants, materialIndex) == 64);
static_assert(offsetof(GeometryPushConstants, entityId) == 68);
static_assert(sizeof(GeometryPushConstants) == 80);
// The only push constant budget every Vulkan implementation guarantees.
static_assert(sizeof(GeometryPushConstants) <= 128);

static_assert(offsetof(GpuMaterial, emissive) == 16);
static_assert(offsetof(GpuMaterial, metallicFactor) == 28);
static_assert(offsetof(GpuMaterial, roughnessFactor) == 32);
static_assert(offsetof(GpuMaterial, normalScale) == 36);
static_assert(offsetof(GpuMaterial, occlusionStrength) == 40);
static_assert(offsetof(GpuMaterial, alphaCutoff) == 44);
static_assert(offsetof(GpuMaterial, baseColorTexture) == 48);
static_assert(offsetof(GpuMaterial, metallicRoughnessTexture) == 52);
static_assert(offsetof(GpuMaterial, normalTexture) == 56);
static_assert(offsetof(GpuMaterial, occlusionTexture) == 60);
static_assert(offsetof(GpuMaterial, emissiveTexture) == 64);
static_assert(sizeof(GpuMaterial) == 80);

static_assert(offsetof(PostProcessPushConstants, encodeSrgb) == 4);
static_assert(offsetof(PostProcessPushConstants, selectedBegin) == 8);
static_assert(offsetof(PostProcessPushConstants, selectedEnd) == 12);
static_assert(offsetof(PostProcessPushConstants, hoveredBegin) == 16);
static_assert(offsetof(PostProcessPushConstants, hoveredEnd) == 20);
static_assert(sizeof(PostProcessPushConstants) == 24);

static_assert(offsetof(DebugDrawPushConstants, color) == 64);
static_assert(sizeof(DebugDrawPushConstants) == 80);

} // namespace GNC
