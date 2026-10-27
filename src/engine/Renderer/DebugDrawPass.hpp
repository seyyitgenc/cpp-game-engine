#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "DebugDraw.hpp"
#include "RenderPass.hpp"
#include "RenderTarget.hpp"
#include "ShaderTypes.hpp"

namespace GNC {

class GeometryPass;
class PostProcessPass;

// Debug draw pass
// DebugDraw's bounding volumes as wireframe: triangle strips drawn with
// polygon mode line, into the scene image after post-processing. Depth tested
// against the G-buffer: at full strength in front of the scene, faint behind
// it. Boxes, spheres and capsules (a cylinder and two hemispheres) are made
// of shared unit meshes, placed by the model matrix; a hull gets a mesh of
// its own, made the first time it is drawn. Skipped when there is nothing to
// draw. Leaves the scene image in eShaderReadOnlyOptimal, as PostProcessPass
// does. shaders/debug_draw.slang.
class DebugDrawPass : public RenderPass {
public:
    // How strongly shapes behind the scene show.
    static constexpr float HiddenOpacity = 0.3f;

private:
    // Host visible, written once when made.
    struct ShapeMesh {
        vk::raii::Buffer vertexBuffer = nullptr;
        vk::raii::DeviceMemory vertexMemory = nullptr;
        vk::raii::Buffer indexBuffer = nullptr;
        vk::raii::DeviceMemory indexMemory = nullptr;
        std::uint32_t indexCount = 0;
    };

    struct HullMesh {
        ShapeMesh mesh;
        std::uint64_t lastDrawn = 0; // frameNumber
    };

    struct Draw {
        const ShapeMesh* mesh;
        DebugDrawPushConstants constants;
    };

    RenderTargetContext context;
    vk::Extent2D extent;
    const DebugDraw& debugDraw;
    GeometryPass* geometryPass;
    PostProcessPass* postProcessPass;

    // [-1, 1] along each axis.
    ShapeMesh unitBox;
    // Radius 1. The hemisphere is the +Y half; the cylinder runs along Y
    // from -1 to 1, open.
    ShapeMesh unitSphere;
    ShapeMesh unitHemisphere;
    ShapeMesh unitCylinder;
    // By DebugDraw::Volume::id.
    std::unordered_map<std::uint64_t, HullMesh> hullMeshes;
    std::uint64_t frameNumber = 0;

    // Rebuilt every frame; a member only to keep its storage.
    std::vector<Draw> draws;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;

public:
    // The draw list must outlive the pass.
    DebugDrawPass(const std::string& name, const RenderTargetContext& renderContext, vk::Extent2D renderExtent,
                  vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders, const DebugDraw& drawList,
                  GeometryPass* gPass, PostProcessPass* pPass);

    void Execute(const FrameContext& frame) override;

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;

private:
    void CreatePipeline(vk::DescriptorSetLayout globalSetLayout, const PassShaders& shaders);
    // Triangle strips, cut apart by RestartIndex.
    ShapeMesh MakeMesh(std::span<const glm::vec3> positions, std::span<const std::uint32_t> indices) const;
    // One draw for each unit shape the volume is made of.
    void AddDraws(const DebugDraw::Volume& volume);
    const ShapeMesh& GetHullMesh(std::uint64_t id, const ConvexHull& hull);
};

} // namespace GNC
