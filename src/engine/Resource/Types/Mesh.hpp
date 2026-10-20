#pragma once

#include "../Resource.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "../../Utils/VulkanHelpers.hpp"
#include "../../Utils/FileSystem.hpp"

namespace GNC {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    // One interleaved buffer at binding 0.
    static vk::VertexInputBindingDescription GetBindingDescription() {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    // Locations match VSInput in shaders/geometry_pass.slang.
    static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        return {{
            { .location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, position) },
            { .location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, normal) },
            { .location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat,    .offset = offsetof(Vertex, texCoord) },
        }};
    }
};

// Geometry on the CPU side, before it goes to the GPU. Also how procedural
// meshes (Resource/Primitives.hpp) are handed to a Mesh.
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

// Mesh resource
// Geometry lives in two GPU buffers. The vertex and index counts are cached at
// load time so drawing never has to ask the buffers how big they are.
//
// Built either from a file (by id) or from MeshData handed in up front:
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

    // Set for procedural meshes. Kept after upload so Unload() + Load() can
    // build the buffers again.
    std::optional<MeshData> procedural;

public:
    explicit Mesh(const std::string& id) : Resource(id) {}
    Mesh(const std::string& id, MeshData data) : Resource(id), procedural(std::move(data)) {}

    const vk::raii::Buffer& GetVertexBuffer() const { return vertexBuffer; }
    const vk::raii::Buffer& GetIndexBuffer() const { return indexBuffer; }

    std::uint32_t GetVertexCount() const { return vertexCount; }
    std::uint32_t GetIndexCount() const { return indexCount; }

    // A procedural mesh has no file to watch.
    std::vector<std::string> GetSourcePaths() const override {
        if (procedural) return {};
        return {SourcePath().string()};
    }

protected:
    bool doLoad() override {
        const ResourceContext& context = GetContext();
        if (!context.transferQueue || !context.transferPool) return false;

        // Parse into CPU vectors first: it is the only place where the data can
        // still be validated cheaply before it is committed to the GPU.
        MeshData loaded;
        if (!procedural && !LoadMeshData(SourcePath(), loaded)) return false;
        const MeshData& data = procedural ? *procedural : loaded;

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
    std::filesystem::path SourcePath() const { return FileSystem::getPath("models/" + GetId() + ".gltf"); }

    // TODO: implement this out (assimp is already in the build, tinygltf if
    // this ends up staying glTF-only)
    bool LoadMeshData(const std::filesystem::path& /*filePath*/, MeshData& /*data*/) {
        return false;
    }

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