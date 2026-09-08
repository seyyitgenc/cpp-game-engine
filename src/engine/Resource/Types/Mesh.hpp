#pragma once

#include "../Resource.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

// TODO: this will want binding/attribute descriptions once there is a pipeline to feed.
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

// Mesh resource
// Geometry lives in two GPU buffers. The vertex and index counts are cached at
// load time so drawing never has to ask the buffers how big they are.
class Mesh : public Resource {
private:
    vk::raii::Buffer vertexBuffer{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory{nullptr};
    std::uint32_t vertexCount = 0;

    vk::raii::Buffer indexBuffer{nullptr};
    vk::raii::DeviceMemory indexBufferMemory{nullptr};
    std::uint32_t indexCount = 0;

public:
    explicit Mesh(const std::string& id) : Resource(id) {}

    const vk::raii::Buffer& GetVertexBuffer() const { return vertexBuffer; }
    const vk::raii::Buffer& GetIndexBuffer() const { return indexBuffer; }

    std::uint32_t GetVertexCount() const { return vertexCount; }
    std::uint32_t GetIndexCount() const { return indexCount; }

protected:
    bool doLoad() override {
        const std::string filePath = "models/" + GetId() + ".gltf";

        // Parse into CPU vectors first: it is the only place where the data can
        // still be validated cheaply before it is committed to the GPU.
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        if (!LoadMeshData(filePath, vertices, indices)) return false;

        CreateVertexBuffer(vertices);
        CreateIndexBuffer(indices);

        vertexCount = static_cast<std::uint32_t>(vertices.size());
        indexCount = static_cast<std::uint32_t>(indices.size());
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
    // TODO: implement these out (assimp is already in the build, tinygltf if
    // this ends up staying glTF-only)
    bool LoadMeshData(const std::string& /*filePath*/,
                      std::vector<Vertex>& /*vertices*/,
                      std::vector<std::uint32_t>& /*indices*/) {
        return false;
    }

    // TODO: staging buffer -> device-local buffer with
    // vk::BufferUsageFlagBits::eVertexBuffer | eTransferDst
    void CreateVertexBuffer(const std::vector<Vertex>& /*vertices*/) {}

    // TODO: same again with eIndexBuffer
    void CreateIndexBuffer(const std::vector<std::uint32_t>& /*indices*/) {}
};
