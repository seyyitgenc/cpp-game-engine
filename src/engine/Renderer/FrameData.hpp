#pragma once

#include <cstdint>

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

// How many frames the CPU may record ahead of the GPU. Anything the CPU
// rewrites every frame (command buffer, camera buffer, ...) needs this many
// copies, so the GPU can still read frame N while the CPU fills frame N+1.
inline constexpr std::uint32_t MaxFramesInFlight = 2;

// Frame data
// Everything one frame in flight owns. The Renderer keeps MaxFramesInFlight of
// these and cycles through them. Once a slot's fence is signaled the GPU is
// done with it, and everything in it can be overwritten.
struct FrameData {
    vk::raii::CommandBuffer commandBuffer = nullptr;
    vk::raii::Fence inFlightFence = nullptr;

    // TODO: once there is a swapchain, renderFinished should be indexed by the
    // acquired swapchain image, not by the frame. Present waits on it, and the
    // only proof that present is done with it is getting that image back from
    // acquire; this frame's fence says nothing about present.
    vk::raii::Semaphore imageAvailableSemaphore = nullptr;
    vk::raii::Semaphore renderFinishedSemaphore = nullptr;

    // Set 0, binding 0: CameraUniform. Host visible and coherent, and mapped
    // for the buffer's whole lifetime, so updating it is a plain memcpy.
    vk::raii::Buffer cameraBuffer = nullptr;
    vk::raii::DeviceMemory cameraMemory = nullptr;
    void* cameraMapped = nullptr;

    vk::raii::DescriptorSet globalSet = nullptr;
};

// Frame context
// What a render pass gets while it records. Non-owning, and only valid during
// the RenderPassManager::Execute() call it was passed to.
struct FrameContext {
    vk::raii::CommandBuffer& commandBuffer;

    // Set 0: per-frame data every pass shares (the camera, for now). Bind it
    // with a pipeline layout whose set 0 is Renderer::GetGlobalSetLayout().
    vk::DescriptorSet globalSet;

    // Which FrameData slot is recording, for passes that keep per-frame
    // resources of their own.
    std::uint32_t frameIndex;
};

} // namespace GNC
