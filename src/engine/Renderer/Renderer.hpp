#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "RenderPassManager.hpp"
#include "Culling/FrustumCulling.hpp"

#include "GeometryPass.hpp"
#include "LightingPass.hpp"
#include "PostProcessPass.hpp"

namespace GNC {

class Renderer {
private:
    const vk::raii::Device* device = nullptr;
    vk::Queue graphicsQueue;
    vk::raii::CommandPool commandPool = nullptr;

    RenderPassManager renderPassManager;
    FrustumCulling cullingSystem;

    // Current frame resources
    vk::raii::CommandBuffer commandBuffer = nullptr;
    vk::raii::Fence fence = nullptr;
    vk::raii::Semaphore imageAvailableSemaphore = nullptr;
    vk::raii::Semaphore renderFinishedSemaphore = nullptr;

public:
    Renderer(const vk::raii::Device* dev, vk::Queue queue, Camera* cam) : device(dev), graphicsQueue(queue), cullingSystem(cam) {
        // Create command pool
        // ...

        // Create synchronization objects
        // ...

        // Set up render passes
        SetupRenderPasses();
    }

    // No need for explicit destructor with RAII objects

    void SetCamera(Camera* camera) {
        cullingSystem.SetCamera(camera);
    }

    void Render(const std::vector<Entity*>& entities) {
        while (device->waitForFences(*fence, vk::True, UINT64_MAX) == vk::Result::eTimeout) {
            // Wait for previous frame to finish
        }

        device->resetFences(*fence);

        // Reset command buffer
        commandBuffer.reset();

        // Perform culling
        cullingSystem.CullScene(entities);

        // Record commands
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffer.begin(beginInfo);

        // Execute render passes
        renderPassManager.Execute(commandBuffer);

        commandBuffer.end();

        // Submit command buffer
        vk::SubmitInfo submitInfo;

        // With vk::raii, we need to dereference the command buffer
        vk::CommandBuffer rawCommandBuffer = *commandBuffer;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&rawCommandBuffer);

        // Set up wait and signal semaphores
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

        // With vk::raii, we need to dereference the semaphores
        vk::Semaphore rawImageAvailableSemaphore = *imageAvailableSemaphore;
        vk::Semaphore rawRenderFinishedSemaphore = *renderFinishedSemaphore;

        submitInfo.setWaitSemaphoreCount(1);
        submitInfo.setPWaitSemaphores(&rawImageAvailableSemaphore);
        submitInfo.setPWaitDstStageMask(waitStages);
        submitInfo.setSignalSemaphoreCount(1);
        submitInfo.setPSignalSemaphores(&rawRenderFinishedSemaphore);

        // With vk::raii, we need to dereference the fence
        vk::Fence rawFence = *fence;
        auto result = graphicsQueue.submit(1, &submitInfo, rawFence);
        if (result != vk::Result::eSuccess) {
            std::printf("failed with result %i", static_cast<int32_t>(result));
        }
    }

private:
    void SetupRenderPasses() {
        // Create geometry pass
        auto geometryPass = renderPassManager.AddRenderPass<GeometryPass>("GeometryPass", &cullingSystem);

        // Create lighting pass
        auto lightingPass = renderPassManager.AddRenderPass<LightingPass>("LightingPass", geometryPass);

        // Create post-process pass
        auto postProcessPass [[maybe_unused]] = renderPassManager.AddRenderPass<PostProcessPass>("PostProcessPass", lightingPass);

        // Add post-process effects
        // ...
    }
};

} // namespace GNC