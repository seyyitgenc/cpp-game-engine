#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Camera/Camera.hpp"
#include "Culling/FrustumCulling.hpp"
#include "FrameData.hpp"
#include "RenderPassManager.hpp"
#include "RenderTarget.hpp"
#include "Utils/VulkanHelpers.hpp"

#include "GeometryPass.hpp"
#include "LightingPass.hpp"
#include "PostProcessPass.hpp"

namespace GNC {

class Renderer {
private:
    // Also what the Renderer hands to the RenderTargets it creates.
    RenderTargetContext context;
    vk::Queue graphicsQueue;
    Camera* camera = nullptr;

    // Declared before the frames on purpose: command buffers and descriptor
    // sets are freed back into these pools, so the pools must outlive them.
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;

    std::array<FrameData, MaxFramesInFlight> frames;
    std::uint32_t currentFrame = 0;

    RenderPassManager renderPassManager;
    FrustumCulling cullingSystem;

public:
    Renderer(const RenderTargetContext& renderContext, vk::Queue queue, std::uint32_t graphicsQueueFamilyIndex, Camera* cam)
        : context(renderContext), graphicsQueue(queue), camera(cam), cullingSystem(cam) {
        CreateCommandPool(graphicsQueueFamilyIndex);
        CreateGlobalDescriptorSetLayout();
        CreateDescriptorPool();
        CreateFrameResources();

        // Set up render passes
        SetupRenderPasses();
    }

    // The GPU may still be reading the last frames' buffers and command
    // buffers; the RAII members must not destroy them out from under it.
    ~Renderer() {
        if (context.device) context.device->waitIdle();
    }

    // Passes hold pointers into the Renderer (&cullingSystem), so it stays put.
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void SetCamera(Camera* cam) {
        camera = cam;
        cullingSystem.SetCamera(cam);
    }

    // Every pass's pipeline layout uses this as set 0.
    const vk::raii::DescriptorSetLayout& GetGlobalSetLayout() const { return globalSetLayout; }

    void Render(const std::vector<Entity*>& entities) {
        FrameData& frame = frames[currentFrame];

        // Once this slot's fence is signaled, the GPU is done with everything in
        // it, so its command buffer and camera buffer can be overwritten.
        while (context.device->waitForFences(*frame.inFlightFence, vk::True, UINT64_MAX) == vk::Result::eTimeout) {
            // Wait for previous frame to finish
        }

        // TODO: acquire the swapchain image here (signals frame.imageAvailableSemaphore).
        // Reset the fence only after acquire succeeds: if it fails and the frame
        // is skipped, an unsignaled fence would block the next wait forever.

        context.device->resetFences(*frame.inFlightFence);

        UpdateCameraBuffer(frame);

        // Perform culling
        cullingSystem.CullScene(entities);

        // Reset command buffer
        frame.commandBuffer.reset();

        // Record commands
        vk::CommandBufferBeginInfo beginInfo;
        frame.commandBuffer.begin(beginInfo);

        // Execute render passes
        const FrameContext frameContext { frame.commandBuffer, *frame.globalSet, currentFrame };
        renderPassManager.Execute(frameContext);

        frame.commandBuffer.end();

        // Submit command buffer
        vk::SubmitInfo submitInfo;

        // With vk::raii, we need to dereference the command buffer
        vk::CommandBuffer rawCommandBuffer = *frame.commandBuffer;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&rawCommandBuffer);

        // Set up wait and signal semaphores
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

        // With vk::raii, we need to dereference the semaphores
        vk::Semaphore rawImageAvailableSemaphore = *frame.imageAvailableSemaphore;
        vk::Semaphore rawRenderFinishedSemaphore = *frame.renderFinishedSemaphore;

        submitInfo.setWaitSemaphoreCount(1);
        submitInfo.setPWaitSemaphores(&rawImageAvailableSemaphore);
        submitInfo.setPWaitDstStageMask(waitStages);
        submitInfo.setSignalSemaphoreCount(1);
        submitInfo.setPSignalSemaphores(&rawRenderFinishedSemaphore);

        // With vk::raii, we need to dereference the fence
        vk::Fence rawFence = *frame.inFlightFence;
        auto result = graphicsQueue.submit(1, &submitInfo, rawFence);
        if (result != vk::Result::eSuccess) {
            std::printf("failed with result %i", static_cast<int32_t>(result));
        }

        // TODO: present here (waits on frame.renderFinishedSemaphore).

        currentFrame = (currentFrame + 1) % MaxFramesInFlight;
    }

private:
    void CreateCommandPool(std::uint32_t queueFamilyIndex) {
        // eResetCommandBuffer lets each frame reset just its own command buffer.
        vk::CommandPoolCreateInfo poolInfo {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndex
        };
        commandPool = vk::raii::CommandPool(*context.device, poolInfo);
    }

    // Set 0, binding 0: the camera. The fragment stage needs it as well, for
    // the camera position in lighting.
    void CreateGlobalDescriptorSetLayout() {
        vk::DescriptorSetLayoutBinding cameraBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        };
        vk::DescriptorSetLayoutCreateInfo layoutInfo {
            .bindingCount = 1,
            .pBindings = &cameraBinding
        };
        globalSetLayout = vk::raii::DescriptorSetLayout(*context.device, layoutInfo);
    }

    void CreateDescriptorPool() {
        vk::DescriptorPoolSize poolSize {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = MaxFramesInFlight
        };
        // eFreeDescriptorSet is required: a vk::raii::DescriptorSet frees
        // itself back into its pool when it is destroyed.
        vk::DescriptorPoolCreateInfo poolInfo {
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = MaxFramesInFlight,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize
        };
        descriptorPool = vk::raii::DescriptorPool(*context.device, poolInfo);
    }

    void CreateFrameResources() {
        const vk::raii::Device& device = *context.device;

        vk::CommandBufferAllocateInfo commandBufferInfo {
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MaxFramesInFlight
        };
        vk::raii::CommandBuffers commandBuffers(device, commandBufferInfo);

        std::array<vk::DescriptorSetLayout, MaxFramesInFlight> setLayouts;
        setLayouts.fill(*globalSetLayout);
        vk::DescriptorSetAllocateInfo setInfo {
            .descriptorPool = *descriptorPool,
            .descriptorSetCount = MaxFramesInFlight,
            .pSetLayouts = setLayouts.data()
        };
        vk::raii::DescriptorSets descriptorSets(device, setInfo);

        for (std::uint32_t i = 0; i < MaxFramesInFlight; ++i) {
            FrameData& frame = frames[i];

            frame.commandBuffer = std::move(commandBuffers[i]);

            // Created signaled, so the first wait in Render() returns at once.
            frame.inFlightFence = vk::raii::Fence(device, vk::FenceCreateInfo { .flags = vk::FenceCreateFlagBits::eSignaled });
            frame.imageAvailableSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo {});
            frame.renderFinishedSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo {});

            createBuffer(device, *context.physicalDevice, sizeof(CameraUniform),
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                frame.cameraBuffer, frame.cameraMemory);
            // Never unmapped by hand: freeing the memory unmaps it.
            frame.cameraMapped = frame.cameraMemory.mapMemory(0, sizeof(CameraUniform));

            frame.globalSet = std::move(descriptorSets[i]);

            vk::DescriptorBufferInfo bufferInfo {
                .buffer = *frame.cameraBuffer,
                .offset = 0,
                .range = sizeof(CameraUniform)
            };
            vk::WriteDescriptorSet write {
                .dstSet = *frame.globalSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &bufferInfo
            };
            device.updateDescriptorSets(write, {});
        }
    }

    // The memory is host coherent, so there is nothing to flush: the submit
    // that follows makes this write visible to the GPU.
    // Y is already flipped inside Camera::GetProjectionMatrix(), so the data
    // goes in as is (no proj[1][1] *= -1 here).
    void UpdateCameraBuffer(FrameData& frame) {
        if (!camera) return;

        const CameraUniform uniform = camera->GetUniform();
        std::memcpy(frame.cameraMapped, &uniform, sizeof(uniform));
    }

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
