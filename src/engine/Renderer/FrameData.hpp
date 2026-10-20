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

    // Signaled by acquire, waited on by this frame's submit. The semaphore the
    // submit signals for present lives in the Swapchain, one per image.
    vk::raii::Semaphore imageAvailableSemaphore = nullptr;

    // Set 0, binding 0: CameraUniform. Host visible and coherent, and mapped
    // for the buffer's whole lifetime, so updating it is a plain memcpy.
    vk::raii::Buffer cameraBuffer = nullptr;
    vk::raii::DeviceMemory cameraMemory = nullptr;
    void* cameraMapped = nullptr;

    // Set 0, binding 1: LightsUniform, kept the same way as the camera buffer.
    vk::raii::Buffer lightsBuffer = nullptr;
    vk::raii::DeviceMemory lightsMemory = nullptr;
    void* lightsMapped = nullptr;

    vk::raii::DescriptorSet globalSet = nullptr;
};

// Frame context
// What a render pass gets while it records. Non-owning, and only valid during
// the RenderPassManager::Execute() call it was passed to.
struct FrameContext {
    vk::raii::CommandBuffer& commandBuffer;

    // Set 0: per-frame data every pass shares (camera and lights). Bind it
    // with a pipeline layout whose set 0 is Renderer::GetGlobalSetLayout().
    vk::DescriptorSet globalSet;

    // Which FrameData slot is recording, for passes that keep per-frame
    // resources of their own.
    std::uint32_t frameIndex;

    // The swapchain image this frame ends up on, as acquired: its contents are
    // undefined. PostProcessPass writes it first, and every pass leaves it in
    // eColorAttachmentOptimal; the Renderer transitions it to ePresentSrcKHR
    // after the last pass.
    vk::Image targetImage;
    vk::ImageView targetView;
};

} // namespace GNC
