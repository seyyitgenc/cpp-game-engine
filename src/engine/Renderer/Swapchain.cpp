#include "Swapchain.hpp"

#include <algorithm>
#include <limits>

#include "VulkanContext.hpp"

namespace GNC {

namespace {

bool IsSrgbFormat(vk::Format format) {
    return format == vk::Format::eB8G8R8A8Srgb || format == vk::Format::eR8G8B8A8Srgb
        || format == vk::Format::eA8B8G8R8SrgbPack32;
}

} // namespace

Swapchain::Swapchain(const VulkanContext& vulkan, vk::Extent2D desiredExtent, const Swapchain* oldSwapchain) {
    const vk::raii::Device& device = vulkan.GetDevice();
    const vk::raii::PhysicalDevice& physicalDevice = vulkan.GetPhysicalDevice();
    const vk::SurfaceKHR surface = *vulkan.GetSurface();

    const vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    surfaceFormat = ChooseSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    extent = ChooseExtent(capabilities, desiredExtent);

    // Three images, so the CPU can record the next frame while one image is on
    // screen and another waits in the queue. maxImageCount 0 means no limit.
    std::uint32_t imageCount = std::max(3u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    swapchain = vk::raii::SwapchainKHR(device, vk::SwapchainCreateInfoKHR {
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        // Rendered to directly by the last passes, never sampled or copied.
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = ChoosePresentMode(physicalDevice.getSurfacePresentModesKHR(surface)),
        .clipped = vk::True,
        // Lets the driver hand resources over from the swapchain being replaced.
        .oldSwapchain = oldSwapchain ? *oldSwapchain->swapchain : vk::SwapchainKHR {}
    });

    images = swapchain.getImages();

    imageViews.reserve(images.size());
    renderFinishedSemaphores.reserve(images.size());
    for (vk::Image image : images) {
        imageViews.emplace_back(device, vk::ImageViewCreateInfo {
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = surfaceFormat.format,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        });
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo {});
    }
}

std::optional<std::uint32_t> Swapchain::AcquireNextImage(vk::Semaphore signal) {
    try {
        const auto [result, imageIndex] = swapchain.acquireNextImage(std::numeric_limits<std::uint64_t>::max(), signal, nullptr);
        // A suboptimal swapchain still hands out an image, and an acquired
        // image has to be presented, so the frame goes ahead and the swapchain
        // is recreated after it.
        if (result == vk::Result::eSuboptimalKHR) suboptimal = true;
        return imageIndex;
    } catch (const vk::OutOfDateKHRError&) {
        return std::nullopt;
    }
}

bool Swapchain::Present(const vk::raii::Queue& queue, std::uint32_t imageIndex) {
    const vk::Semaphore waitSemaphore = *renderFinishedSemaphores[imageIndex];
    const vk::SwapchainKHR handle = *swapchain;
    const vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &handle,
        .pImageIndices = &imageIndex
    };

    try {
        if (queue.presentKHR(presentInfo) == vk::Result::eSuboptimalKHR) suboptimal = true;
    } catch (const vk::OutOfDateKHRError&) {
        // Even a rejected present still waits on the semaphore, so there is
        // nothing to clean up here.
        return false;
    }
    return !suboptimal;
}

bool Swapchain::IsSrgb() const {
    return IsSrgbFormat(surfaceFormat.format);
}

vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available) {
    // UNORM, with the post-process pass doing the sRGB encode itself. ImGui's
    // colors are already sRGB encoded and meant to be written and blended as
    // they are; an sRGB format would encode them a second time and wash the
    // UI out. The ImGui backend picks these same formats for the windows it
    // creates when a panel is dragged out of the main one.
    for (const vk::SurfaceFormatKHR& format : available) {
        if ((format.format == vk::Format::eB8G8R8A8Unorm || format.format == vk::Format::eR8G8B8A8Unorm)
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    // Anything else still shows the scene correctly (IsSrgb() tells the
    // post-process pass whether to encode), but an sRGB one brightens the UI.
    return available.front();
}

vk::PresentModeKHR Swapchain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& available) {
    // Mailbox: no tearing, and the newest frame replaces a queued one instead
    // of waiting behind it. FIFO (vsync) is the one mode that always exists.
    if (std::ranges::find(available, vk::PresentModeKHR::eMailbox) != available.end()) {
        return vk::PresentModeKHR::eMailbox;
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D desired) {
    // UINT32_MAX means the surface takes its size from the swapchain;
    // anything else is the size it has to be.
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    return {
        std::clamp(desired.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(desired.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

} // namespace GNC
