#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Utils/VulkanHelpers.hpp"

namespace GNC {

// Non-owning. The device must outlive every render target built from it.
struct RenderTargetContext {
    const vk::raii::Device* device = nullptr;
    const vk::raii::PhysicalDevice* physicalDevice = nullptr;
};

// One image a pass renders into and a later pass reads: device local memory,
// the image, and a view over all of it (the G-buffer, the HDR lighting output).
struct Attachment {
    // Declared memory first so it is released last, after the image bound to it.
    vk::raii::DeviceMemory memory = nullptr;
    vk::raii::Image image = nullptr;
    vk::raii::ImageView view = nullptr;
    vk::Format format = vk::Format::eUndefined;

    Attachment() = default;

    Attachment(const RenderTargetContext& context, vk::Extent2D extent, vk::Format imageFormat,
               vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect)
        : format(imageFormat) {
        createImage(*context.device, *context.physicalDevice, extent.width, extent.height,
            format, vk::ImageTiling::eOptimal, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, image, memory);
        view = createImageView(*context.device, image, format, aspect);
    }
};

} // namespace GNC