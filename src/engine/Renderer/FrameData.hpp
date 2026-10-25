#pragma once

#include <cstdint>

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

// How many frames the CPU may record ahead of the GPU; whatever it rewrites
// every frame needs this many copies.
inline constexpr std::uint32_t MaxFramesInFlight = 2;

// Frame data
// What one frame in flight owns, free to overwrite once its fence signals.
struct FrameData {
    vk::raii::CommandBuffer commandBuffer = nullptr;
    vk::raii::Fence inFlightFence = nullptr;

    // The semaphore for present is the Swapchain's, one per image.
    vk::raii::Semaphore imageAvailableSemaphore = nullptr;

    // Set 0, binding 0: CameraUniform. Persistently mapped, host coherent.
    vk::raii::Buffer cameraBuffer = nullptr;
    vk::raii::DeviceMemory cameraMemory = nullptr;
    void* cameraMapped = nullptr;

    // Set 0, binding 1: LightsUniform, kept the same way.
    vk::raii::Buffer lightsBuffer = nullptr;
    vk::raii::DeviceMemory lightsMemory = nullptr;
    void* lightsMapped = nullptr;

    vk::raii::DescriptorSet globalSet = nullptr;
};

// Frame context
// What a render pass gets while it records. Non-owning, valid only during
// RenderPassManager::Execute().
struct FrameContext {
    vk::raii::CommandBuffer& commandBuffer;

    // Camera and lights; bind with a layout whose set 0 is
    // Renderer::GetGlobalSetLayout().
    vk::DescriptorSet globalSet;

    std::uint32_t frameIndex;

    // The swapchain image, as acquired (contents undefined). Passes leave it
    // in eColorAttachmentOptimal; the Renderer transitions it for present.
    vk::Image targetImage;
    vk::ImageView targetView;
};

} // namespace GNC
