#pragma once

#include "../Resource.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "../../Utils/VulkanHelpers.hpp"

namespace GNC {

struct Vertex {
    glm::vec3 position {0.0f};
    glm::vec3 normal {0.0f, 1.0f, 0.0f};
    glm::vec2 texCoord {0.0f};
    // glTF's: xyz along +u, w the handedness, so the bitangent is
    // cross(normal, tangent.xyz) * w.
    glm::vec4 tangent {1.0f, 0.0f, 0.0f, 1.0f};

    // One interleaved buffer at binding 0.
    static vk::VertexInputBindingDescription GetBindingDescription() {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    // Locations match VSInput in shaders/geometry_pass.slang.
    static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescriptions() {
        return {{
            { .location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,    .offset = offsetof(Vertex, position) },
            { .location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat,    .offset = offsetof(Vertex, normal) },
            { .location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat,       .offset = offsetof(Vertex, texCoord) },
            { .location = 3, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(Vertex, tangent) },
        }};
    }
};

// CPU-side geometry; also how procedural meshes (Resource/Primitives.hpp)
// reach a Mesh.
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

// Tangents from positions, normals and texture coordinates (Lengyel's
// method), orthogonalized against the normals. glTF asks for MikkTSpace when
// a file has none; this is close but not exact, so a baked normal map can
// show faint seams. The bitangent runs along -v: glTF's v runs down the
// image, its normal maps have +Y up (matching what Blender's exporter
// writes).
inline void GenerateTangents(MeshData& data) {
    std::vector<glm::vec3> tangents(data.vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(data.vertices.size(), glm::vec3(0.0f));

    for (std::size_t i = 0; i + 2 < data.indices.size(); i += 3) {
        const std::uint32_t corners[3] = { data.indices[i], data.indices[i + 1], data.indices[i + 2] };
        const Vertex& a = data.vertices[corners[0]];
        const Vertex& b = data.vertices[corners[1]];
        const Vertex& c = data.vertices[corners[2]];

        const glm::vec3 edge1 = b.position - a.position;
        const glm::vec3 edge2 = c.position - a.position;
        const glm::vec2 uv1 = (b.texCoord - a.texCoord) * glm::vec2(1.0f, -1.0f);
        const glm::vec2 uv2 = (c.texCoord - a.texCoord) * glm::vec2(1.0f, -1.0f);

        const float determinant = uv1.x * uv2.y - uv2.x * uv1.y;
        if (std::abs(determinant) < 1e-12f) continue;

        // Unnormalized, so larger triangles weigh more.
        const glm::vec3 tangent = (edge1 * uv2.y - edge2 * uv1.y) / determinant;
        const glm::vec3 bitangent = (edge2 * uv1.x - edge1 * uv2.x) / determinant;
        for (const std::uint32_t corner : corners) {
            tangents[corner] += tangent;
            bitangents[corner] += bitangent;
        }
    }

    for (std::size_t i = 0; i < data.vertices.size(); ++i) {
        Vertex& vertex = data.vertices[i];
        glm::vec3 tangent = tangents[i] - vertex.normal * glm::dot(vertex.normal, tangents[i]);
        // No texture coordinates to go by: any direction in the surface.
        if (glm::dot(tangent, tangent) < 1e-12f) {
            const glm::vec3 axis = std::abs(vertex.normal.x) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            tangent = glm::cross(axis, vertex.normal);
        }
        tangent = glm::normalize(tangent);
        const float handedness = glm::dot(glm::cross(vertex.normal, tangent), bitangents[i]) < 0.0f ? -1.0f : 1.0f;
        vertex.tangent = glm::vec4(tangent, handedness);
    }
}

// Mesh resource
// Built from MeshData handed in up front, procedural or imported
// (Scene/ModelImporter):
//     resources.Load<Mesh>("sphere", CreateSphere(1.0f, 64, 128));
// Uploading needs the transfer queue and pool in the ResourceContext.
class Mesh : public Resource {
private:
    vk::raii::Buffer vertexBuffer{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory{nullptr};
    std::uint32_t vertexCount = 0;

    vk::raii::Buffer indexBuffer{nullptr};
    vk::raii::DeviceMemory indexBufferMemory{nullptr};
    std::uint32_t indexCount = 0;

    // Kept after upload, so Unload() + Load() can rebuild the buffers.
    MeshData data;

public:
    Mesh(const std::string& id, MeshData meshData) : Resource(id), data(std::move(meshData)) {}

    const vk::raii::Buffer& GetVertexBuffer() const { return vertexBuffer; }
    const vk::raii::Buffer& GetIndexBuffer() const { return indexBuffer; }

    std::uint32_t GetVertexCount() const { return vertexCount; }
    std::uint32_t GetIndexCount() const { return indexCount; }

    // In model space, loaded or not.
    const std::vector<Vertex>& GetVertices() const { return data.vertices; }

protected:
    bool doLoad() override {
        const ResourceContext& context = GetContext();
        if (!context.transferQueue || !context.transferPool) return false;

        if (data.vertices.empty() || data.indices.empty()) return false;

        CreateVertexBuffer(data.vertices);
        CreateIndexBuffer(data.indices);

        vertexCount = static_cast<std::uint32_t>(data.vertices.size());
        indexCount = static_cast<std::uint32_t>(data.indices.size());
        return true;
    }

    void doUnload() override {
        indexBuffer = nullptr;
        indexBufferMemory = nullptr;
        vertexBuffer = nullptr;
        vertexBufferMemory = nullptr;

        vertexCount = 0;
        indexCount = 0;
    }

private:
    void CreateVertexBuffer(const std::vector<Vertex>& vertices) {
        const ResourceContext& context = GetContext();
        uploadBuffer(GetDevice(), *context.physicalDevice, *context.transferQueue, *context.transferPool,
            vertices.data(), sizeof(Vertex) * vertices.size(), vk::BufferUsageFlagBits::eVertexBuffer,
            vertexBuffer, vertexBufferMemory);
    }

    void CreateIndexBuffer(const std::vector<std::uint32_t>& indices) {
        const ResourceContext& context = GetContext();
        uploadBuffer(GetDevice(), *context.physicalDevice, *context.transferQueue, *context.transferPool,
            indices.data(), sizeof(std::uint32_t) * indices.size(), vk::BufferUsageFlagBits::eIndexBuffer,
            indexBuffer, indexBufferMemory);
    }
};

} // namespace GNC