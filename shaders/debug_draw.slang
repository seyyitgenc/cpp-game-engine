// Debug draw (Renderer/DebugDrawPass): bounding volume shapes as wireframe
// over the scene image, depth tested against the G-buffer. Colors arrive sRGB
// encoded, as the scene image holds them, and go out as they are.

import common.frame;

// Per draw. The C++ half is DebugDrawPushConstants (Renderer/ShaderTypes.hpp).
struct DrawData {
    float4x4 model;
    float4 color;
};

[[vk::push_constant]] ConstantBuffer<DrawData> draw;

// A wire lying on a surface would fight it for depth, so every point moves
// this share of its distance towards the camera: along its view ray in
// perspective, along the view axis in orthographic, so it stays on its pixel.
static const float DEPTH_PULL = 0.001;

// DebugDrawPass's vertex input: model space positions.
struct VSInput {
    [[vk::location(0)]] float3 position;
};

[shader("vertex")]
float4 vertMain(VSInput input) : SV_Position {
    float4 viewPosition = mul(camera.view, mul(draw.model, float4(input.position, 1.0)));
    // A perspective projection's last row is (0, 0, -1, 0).
    if (camera.projection[3][3] == 0.0) {
        viewPosition.xyz *= 1.0 - DEPTH_PULL;
    } else {
        viewPosition.z += abs(viewPosition.z) * DEPTH_PULL;
    }
    return mul(camera.projection, viewPosition);
}

[shader("fragment")]
float4 fragMain() : SV_Target {
    return draw.color;
}
