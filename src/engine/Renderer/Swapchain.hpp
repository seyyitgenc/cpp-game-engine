#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

class VulkanContext;

// Swapchain
// The images the window shows, plus one "render finished" semaphore per image.
//
// Those semaphores are per image, not per frame in flight: present waits on
// one, and the only proof that present is done with it is getting the same
// image back from a later acquire. A frame's fence says nothing about present.
//
// Resizing means building a new Swapchain (passing the old one in) once the
// GPU is idle; nothing in here is ever patched in place.
class Swapchain {
private:
    vk::raii::SwapchainKHR swapchain = nullptr;
    std::vector<vk::Image> images; // owned by the swapchain, not destroyed by us
    std::vector<vk::raii::ImageView> imageViews;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;

    // Set when acquire or present said eSuboptimalKHR. Still usable, but it no
    // longer matches the surface exactly, so it is due for recreation.
    bool suboptimal = false;

public:
    // desiredExtent is only used when the surface lets the application pick
    // (otherwise the surface's own size wins). Must not be 0x0.
    Swapchain(const VulkanContext& vulkan, vk::Extent2D desiredExtent, const Swapchain* oldSwapchain = nullptr);

    // Signals `signal` once the image may be rendered to. nullopt when the
    // swapchain is out of date: recreate it and skip the frame.
    std::optional<std::uint32_t> AcquireNextImage(vk::Semaphore signal);

    // Queues the image for display once its render finished semaphore is
    // signaled. False when the swapchain should be recreated before the next
    // frame (out of date or suboptimal).
    bool Present(const vk::raii::Queue& queue, std::uint32_t imageIndex);

    vk::Image GetImage(std::uint32_t index) const { return images[index]; }
    vk::ImageView GetImageView(std::uint32_t index) const { return *imageViews[index]; }
    vk::Semaphore GetRenderFinishedSemaphore(std::uint32_t index) const { return *renderFinishedSemaphores[index]; }

    vk::Format GetFormat() const { return surfaceFormat.format; }
    vk::Extent2D GetExtent() const { return extent; }

    // Whether the format encodes to sRGB on write. If not, whoever renders to
    // it has to apply the transfer function itself.
    bool IsSrgb() const;

private:
    static vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available);
    static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& available);
    static vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D desired);
};

} // namespace GNC
