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
// Owns the swapchain and the frame loop around the render passes: per-frame
// resources, acquire, record, submit, present. Deferred: GeometryPass fills a
// G-buffer, LightingPass shades it into HDR, PostProcessPass tonemaps that
// into the scene image, and ImGuiPass draws the UI, which shows the scene
// image in its Viewport panel, into the swapchain image.
//
// Knows nothing about the window. When Render() reports the swapchain out of
// date, or the window was resized, the owner calls Resize() with the new
// framebuffer size. The scene has a size of its own, SetSceneExtent().
class Renderer {
private:
    const VulkanContext& vulkan;
    // Also what the Renderer hands to the render targets it creates.
    RenderTargetContext context;
    Camera* camera = nullptr;

    // Flat ambient light, until there is image based lighting. Linear RGB,
    // already multiplied by its intensity.
    glm::vec3 ambientColor {0.03f};
    bool warnedAboutLightLimit = false;

    // Declared before the frames on purpose: command buffers and descriptor
    // sets are freed back into these pools, so the pools must outlive them.
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;

    std::array<FrameData, MaxFramesInFlight> frames;
    std::uint32_t currentFrame = 0;

    std::unique_ptr<Swapchain> swapchain;

    // Every offscreen target (G-buffer, lighting output, scene image) is this
    // size, and the camera's aspect ratio follows it.
    vk::Extent2D sceneExtent;
    // Counts the times the passes were built, which replaces the scene image.
    std::uint64_t sceneVersion = 0;

    RenderPassManager renderPassManager;
    // Owned by renderPassManager.
    PostProcessPass* postProcessPass = nullptr;
    FrustumCulling cullingSystem;

public:
    // extent: the window's framebuffer size, in pixels. Must not be 0x0.
    Renderer(const VulkanContext& vulkanContext, vk::Extent2D extent, Camera* cam);

    // The GPU may still be reading the last frames' buffers and command
    // buffers; the RAII members must not destroy them out from under it.
    ~Renderer();

    // Passes hold pointers into the Renderer (&cullingSystem), so it stays put.
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void SetCamera(Camera* cam);

    // Linear RGB, already multiplied by its intensity.
    void SetAmbientLight(const glm::vec3& color) { ambientColor = color; }

    // Every pass's pipeline layout uses this as set 0.
    const vk::raii::DescriptorSetLayout& GetGlobalSetLayout() const { return globalSetLayout; }

    // The swapchain's format, which ImGui's pipeline for the main window is
    // built for. Can change when Resize() rebuilds the swapchain.
    vk::Format GetTargetFormat() const { return swapchain->GetFormat(); }

    // Draws and presents one frame. Returns false when the swapchain no longer
    // matches the window; call Resize() before the next frame.
    bool Render(const std::vector<Entity*>& entities);

    // Rebuilds the swapchain and every render pass. Waits for the GPU first.
    // extent must not be 0x0 (minimized): skip frames until it is not.
    void Resize(vk::Extent2D extent);

    // -----
    // Scene
    // -----

    // The size the scene renders at, in pixels; at least 1x1. When it
    // changes, waits for the GPU and rebuilds every render pass.
    void SetSceneExtent(vk::Extent2D extent);
    vk::Extent2D GetSceneExtent() const { return sceneExtent; }

    // The finished frame: tonemapped, sRGB encoded RGBA8, sceneExtent in
    // size, and in eShaderReadOnlyOptimal outside the post-process pass.
    // Replaced whenever the passes are rebuilt, which GetSceneVersion()
    // counts; the GPU is idle when that happens.
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
