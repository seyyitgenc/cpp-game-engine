#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Culling/FrustumCulling.hpp"
#include "FrameData.hpp"
#include "RenderPassManager.hpp"
#include "RenderTarget.hpp"
#include "ShaderTypes.hpp"
#include "Swapchain.hpp"

namespace GNC {

class Camera;
class Entity;
class LightComponent;
class PostProcessPass;
class TransformComponent;
class VulkanContext;

// Renderer
// Owns the swapchain, the frames in flight and the render passes. Knows
// nothing about the window: when Render() returns false or the window
// resizes, the owner calls Resize(). The scene has a size of its own,
// SetSceneExtent().
class Renderer {
private:
    const VulkanContext& vulkan;
    RenderTargetContext context;
    Camera* camera = nullptr;

    // Flat, until there is image based lighting.
    glm::vec3 ambientColor {0.03f};
    bool warnedAboutLightLimit = false;

    // Before the frames: their command buffers and sets are freed into these
    // pools.
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;

    std::array<FrameData, MaxFramesInFlight> frames;
    std::uint32_t currentFrame = 0;

    std::unique_ptr<Swapchain> swapchain;

    // Every offscreen target is this size; the camera's aspect follows it.
    vk::Extent2D sceneExtent;
    // Bumped by every pass rebuild, which replaces the scene image.
    std::uint64_t sceneVersion = 0;

    RenderPassManager renderPassManager;
    // Owned by renderPassManager.
    PostProcessPass* postProcessPass = nullptr;
    FrustumCulling cullingSystem;

public:
    // extent: the framebuffer size; not 0x0.
    Renderer(const VulkanContext& vulkanContext, vk::Extent2D extent, Camera* cam);

    // Waits for the GPU: frames in flight still use the RAII members.
    ~Renderer();

    // Passes hold pointers into the Renderer (&cullingSystem).
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void SetCamera(Camera* cam);

    // Linear RGB, already multiplied by its intensity.
    void SetAmbientLight(const glm::vec3& color) { ambientColor = color; }

    // Set 0 of every pass's pipeline layout.
    const vk::raii::DescriptorSetLayout& GetGlobalSetLayout() const { return globalSetLayout; }

    // What ImGui's main-window pipeline is built for; Resize() can change it.
    vk::Format GetTargetFormat() const { return swapchain->GetFormat(); }

    // Returns false when the swapchain no longer matches the window; call
    // Resize() before the next frame.
    bool Render(const std::vector<Entity*>& entities);

    // Waits for the GPU, then rebuilds the swapchain and every render pass.
    // Not with 0x0 (minimized).
    void Resize(vk::Extent2D extent);

    // -----
    // Scene
    // -----

    // At least 1x1. A change waits for the GPU and rebuilds every pass.
    void SetSceneExtent(vk::Extent2D extent);
    vk::Extent2D GetSceneExtent() const { return sceneExtent; }

    // Tonemapped, sRGB-encoded RGBA8, in eShaderReadOnlyOptimal outside the
    // post-process pass. Replaced, with the GPU idle, by every pass rebuild,
    // which GetSceneVersion() counts.
    vk::ImageView GetSceneView() const;
    std::uint64_t GetSceneVersion() const { return sceneVersion; }

private:
    void CreateCommandPool();
    void CreateGlobalDescriptorSetLayout();
    void CreateDescriptorPool();
    void CreateFrameResources();

    void UpdateCameraBuffer(FrameData& frame);
    void UpdateLightsBuffer(FrameData& frame, const std::vector<Entity*>& entities);
    static GpuLight MakeGpuLight(const LightComponent& light, const TransformComponent* transform);

    void UpdateCameraAspect();
    // The GPU must be idle.
    void RebuildRenderPasses();
    void SetupRenderPasses();
};

} // namespace GNC
