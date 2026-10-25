#include "VulkanHelpers.hpp"

#include <cstring>
#include <stdexcept>

namespace GNC {

uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

vk::raii::ImageView createImageView(const vk::raii::Device &device, vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspect)
{
    vk::ImageViewCreateInfo viewInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            aspect,
            0, 1,
            0, 1 }
    };
    return vk::raii::ImageView(device, viewInfo);
}

void createImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageInfo {
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {
            width,
            height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    image = vk::raii::Image(device, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo {
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
    };
    imageMemory = vk::raii::DeviceMemory(device, allocInfo);
    image.bindMemory(imageMemory, 0);
}

void createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    buffer = vk::raii::Buffer(device, bufferInfo);

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo {
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
    };
    bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    buffer.bindMemory(bufferMemory, 0);
}

void uploadBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::Queue& queue, const vk::raii::CommandPool& commandPool, const void* data, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
{
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingMemory = nullptr;
    createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, stagingMemory);

    void* mapped = stagingMemory.mapMemory(0, size);
    std::memcpy(mapped, data, static_cast<std::size_t>(size));
    stagingMemory.unmapMemory();

    createBuffer(device, physicalDevice, size, usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, bufferMemory);

    vk::raii::CommandBuffers commandBuffers(device, vk::CommandBufferAllocateInfo {
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    });
    const vk::raii::CommandBuffer& commandBuffer = commandBuffers.front();

    commandBuffer.begin(vk::CommandBufferBeginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    commandBuffer.copyBuffer(*stagingBuffer, *buffer, vk::BufferCopy { .size = size });

    // Waiting for the queue only makes the copy available; the barrier makes
    // it visible to later reads on this queue, in any submission.
    const vk::MemoryBarrier2 barrier {
        .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = vk::AccessFlagBits2::eMemoryRead
    };
    commandBuffer.pipelineBarrier2(vk::DependencyInfo { .memoryBarrierCount = 1, .pMemoryBarriers = &barrier });
    commandBuffer.end();

    const vk::CommandBuffer rawCommandBuffer = *commandBuffer;
    queue.submit(vk::SubmitInfo { .commandBufferCount = 1, .pCommandBuffers = &rawCommandBuffer });
    // The staging buffer dies with this function.
    queue.waitIdle();
}

vk::ImageMemoryBarrier2 makeImageBarrier(vk::Image image, vk::ImageAspectFlags aspect, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess, vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess)
{
    return {
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = {
            aspect,
            0, 1,
            0, 1 }
    };
}

} // namespace GNC