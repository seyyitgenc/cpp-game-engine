#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

vk::raii::ImageView createImageView(const vk::raii::Device& device, vk::raii::Image& image, vk::Format format,
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor);

void createImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image,
    vk::raii::DeviceMemory& imageMemory);

void createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer,
    vk::raii::DeviceMemory& bufferMemory);

// Creates a device local buffer and fills it with size bytes from data, through
// a host visible staging buffer. Blocks until the copy is done, and leaves the
// result visible to any later work on the queue. Meant for load time; per-frame
// uploads would want batching instead of a stall per call.
void uploadBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Queue& queue, const vk::raii::CommandPool& commandPool,
    const void* data, vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);

// One layout transition for a whole single-mip, single-layer image, in
// synchronization2 terms: src is the work that has to finish first, dst the
// work that waits for it. Batch several into one pipelineBarrier2() call.
vk::ImageMemoryBarrier2 makeImageBarrier(vk::Image image, vk::ImageAspectFlags aspect,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
    vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess);

} // namespace GNC