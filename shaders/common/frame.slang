// Set 0, shared by every pass (Renderer::CreateGlobalDescriptorSetLayout()).
// The C++ halves, which must match byte for byte, are CameraUniform
// (Camera/Camera.hpp) and GpuLight / LightsUniform (Renderer/ShaderTypes.hpp).

// Renderer/ShaderTypes.hpp: MaxLights
static const uint MAX_LIGHTS = 64;

// ECS/Component/LightComponent.hpp: LightType
static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;

struct CameraData {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    // Clip space back to world space, for rebuilding positions from depth.
    float4x4 inverseViewProjection;
    float4 position; // w unused
};

// Every float3 is followed by a scalar, so std140 packs each pair into one
// 16 byte slot and the struct needs no hidden padding.
struct GpuLight {
    float3 position;  // world space; unused by directional lights
    uint type;        // LIGHT_TYPE_*
    float3 direction; // world space, unit length, the way the light shines
    float range;      // point/spot: distance at which it fades to zero, 0 = no cutoff
    float3 color;     // linear RGB
    float intensity;  // candela for point/spot, lux for directional
    // Spot cone falloff folded into one multiply-add, see SpotAttenuation().
    float spotAngleScale;
    float spotAngleOffset;
    float2 _pad;
};

struct LightsData {
    float3 ambientColor; // stand-in for image based lighting, see lighting_pass
    uint lightCount;
    GpuLight lights[MAX_LIGHTS];
};

[[vk::binding(0, 0)]] ConstantBuffer<CameraData> camera;
[[vk::binding(1, 0)]] ConstantBuffer<LightsData> lighting;
