#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

class VulkanContext;

// Swapchain
// The window's images, plus a render-finished semaphore per image, not per
// frame: only a later acquire of the same image proves present is done with
// it. Resizing builds a new Swapchain once the GPU is idle.
class Swapchain {
private:
    vk::raii::SwapchainKHR swapchain = nullptr;
    std::vector<vk::Image> images; // owned by the swapchain, not destroyed by us
    std::vector<vk::raii::ImageView> imageViews;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;

    // Acquire or present said eSuboptimalKHR: usable, but due for recreation.
    bool suboptimal = false;

public:
    // desiredExtent applies only if the surface lets the application pick.
    // Not 0x0.
    Swapchain(const VulkanContext& vulkan, vk::Extent2D desiredExtent, const Swapchain* oldSwapchain = nullptr);

    // Signals `signal` once the image may be rendered to. nullopt when the
    // swapchain is out of date: recreate it and skip the frame.
    std::optional<std::uint32_t> AcquireNextImage(vk::Semaphore signal);

    // False when the swapchain should be recreated before the next frame.
    bool Present(const vk::raii::Queue& queue, std::uint32_t imageIndex);

    vk::Image GetImage(std::uint32_t index) const { return images[index]; }
    vk::ImageView GetImageView(std::uint32_t index) const { return *imageViews[index]; }
    vk::Semaphore GetRenderFinishedSemaphore(std::uint32_t index) const { return *renderFinishedSemaphores[index]; }

    vk::Format GetFormat() const { return surfaceFormat.format; }
    vk::Extent2D GetExtent() const { return extent; }

    // Whether the format encodes to sRGB on write.
    bool IsSrgb() const;

private:
    static vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available);
    static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& available);
    static vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D desired);
};

} // namespace GNC
