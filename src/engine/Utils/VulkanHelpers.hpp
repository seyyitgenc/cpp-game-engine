#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

vk::raii::ImageView createImageView(const vk::raii::Device& device, vk::raii::Image& image, vk::Format format);

void createImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image,
    vk::raii::DeviceMemory& imageMemory);

} // namespace GNC