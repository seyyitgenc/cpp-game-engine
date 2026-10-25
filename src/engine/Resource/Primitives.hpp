#pragma once

#include <cmath>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "Types/Mesh.hpp"

namespace GNC {

// Procedural meshes, as MeshData ready to hand to a Mesh.

// UV sphere centered on the origin. stacks run pole to pole (top is +Y),
// slices around the Y axis. The seam column is duplicated so texture
// coordinates can run 0..1 without wrapping. Triangles are counter-clockwise
// seen from outside, like glTF.
inline MeshData CreateSphere(float radius, std::uint32_t stacks, std::uint32_t slices) {
    MeshData data;
    data.vertices.reserve((stacks + 1) * (slices + 1));
    data.indices.reserve(stacks * slices * 6);

    for (std::uint32_t i = 0; i <= stacks; ++i) {
        const float theta = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(stacks);
        for (std::uint32_t j = 0; j <= slices; ++j) {
            const float phi = glm::two_pi<float>() * static_cast<float>(j) / static_cast<float>(slices);
            // phi = 0 faces +Z; phi grows towards +X.
            const glm::vec3 normal {
                std::sin(theta) * std::sin(phi),
                std::cos(theta),
                std::sin(theta) * std::cos(phi)
            };
            data.vertices.push_back({
                .position = normal * radius,
                .normal = normal,
                .texCoord = { static_cast<float>(j) / static_cast<float>(slices), static_cast<float>(i) / static_cast<float>(stacks) }
            });
        }
    }

    // Per quad: a top-left, b below it, c below-right, d right.
    for (std::uint32_t i = 0; i < stacks; ++i) {
        for (std::uint32_t j = 0; j < slices; ++j) {
            const std::uint32_t a = i * (slices + 1) + j;
            const std::uint32_t b = a + slices + 1;
            const std::uint32_t c = b + 1;
            const std::uint32_t d = a + 1;
            data.indices.insert(data.indices.end(), { a, b, c, a, c, d });
        }
    }
    GenerateTangents(data);
    return data;
}

// Square of the given side length in the XZ plane, centered on the origin,
// facing +Y. Texture coordinates run 0..1 across it.
inline MeshData CreatePlane(float size) {
    const float half = size * 0.5f;
    const glm::vec3 up { 0.0f, 1.0f, 0.0f };

    MeshData data;
    data.vertices = {
        { .position = { -half, 0.0f, -half }, .normal = up, .texCoord = { 0.0f, 0.0f } },
        { .position = { -half, 0.0f,  half }, .normal = up, .texCoord = { 0.0f, 1.0f } },
        { .position = {  half, 0.0f,  half }, .normal = up, .texCoord = { 1.0f, 1.0f } },
        { .position = {  half, 0.0f, -half }, .normal = up, .texCoord = { 1.0f, 0.0f } },
    };
    // Counter-clockwise seen from above.
    data.indices = { 0, 1, 2, 0, 2, 3 };
    GenerateTangents(data);
    return data;
}

} // namespace GNC
