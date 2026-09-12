#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Utils/VulkanHelpers.hpp"

namespace GNC {

struct RenderTargetContext {
    const vk::raii::Device* device = nullptr;
    const vk::raii::PhysicalDevice* physicalDevice = nullptr;
};

// Render target class
class RenderTarget {
private:
    const RenderTargetContext* m_context = nullptr;

    vk::raii::Image colorImage = nullptr;
    vk::raii::DeviceMemory colorMemory = nullptr;
    vk::raii::ImageView colorImageView = nullptr;

    vk::raii::Image depthImage = nullptr;
    vk::raii::DeviceMemory depthMemory = nullptr;
    vk::raii::ImageView depthImageView = nullptr;

    uint32_t width;
    uint32_t height;

public:
    RenderTarget(uint32_t w, uint32_t h) : width(w), height(h) {
        // Create color and depth images
        CreateColorResources();
        CreateDepthResources();

        // Note: With dynamic rendering, we don't need to create VkRenderPass
        // or VkFramebuffer objects. Instead, we just create the images and
        // image views that will be used directly with vkCmdBeginRendering.
    }

    void SetContext(const RenderTargetContext* context) {
        m_context = context;
    }

    // No need for explicit destructor with RAII objects

    vk::ImageView GetColorImageView() const { return *colorImageView; }
    vk::ImageView GetDepthImageView() const { return *depthImageView; }

    uint32_t GetWidth() const { return width; }
    uint32_t GetHeight() const { return height; }

private:
    void CreateColorResources() {
        createImage(*m_context->device, *m_context->physicalDevice, 1920, 1080,
            vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
             vk::MemoryPropertyFlagBits::eDeviceLocal, colorImage, colorMemory);

        colorImageView = createImageView(*m_context->device, colorImage, vk::Format::eR8G8B8A8Srgb);
    }

    void CreateDepthResources() {
        // Implementation to create depth image, memory, and view
        // With dynamic rendering, we just need to create the image and image view
        // that will be used with vkCmdBeginRendering
        // ...
    }

};

} // namespace GNC