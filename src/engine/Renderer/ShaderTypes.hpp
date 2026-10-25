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
    glm::vec4 baseColorFactor {1.0f};
    glm::vec4 emissive {0.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float pad[2] {};
};

// shaders/post_process.slang: PostData (push constants)
struct PostProcessPushConstants {
    float exposure = 1.0f;
    std::uint32_t encodeSrgb = 0;
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

static_assert(offsetof(GeometryPushConstants, baseColorFactor) == 64);
static_assert(offsetof(GeometryPushConstants, emissive) == 80);
static_assert(offsetof(GeometryPushConstants, metallicFactor) == 96);
static_assert(offsetof(GeometryPushConstants, roughnessFactor) == 100);
static_assert(sizeof(GeometryPushConstants) == 112);
// The only push constant budget every Vulkan implementation guarantees.
static_assert(sizeof(GeometryPushConstants) <= 128);

static_assert(offsetof(PostProcessPushConstants, encodeSrgb) == 4);
static_assert(sizeof(PostProcessPushConstants) == 8);

} // namespace GNC
