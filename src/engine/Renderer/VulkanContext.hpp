#pragma once

#include <cstdint>

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

class Window;

// Vulkan context
// The application-lifetime objects: instance, debug messenger, surface, GPU,
// device, and its one queue for graphics, present and uploads. Requires
// Vulkan 1.3 with dynamic rendering and synchronization2.
class VulkanContext {
private:
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue queue = nullptr;
    std::uint32_t queueFamilyIndex = 0;

    // Debug builds only, and only if the layer is installed.
    bool validationEnabled = false;

public:
    // The window must outlive the context: the surface belongs to it.
    explicit VulkanContext(const Window& window);

    // Everything else holds pointers into this object.
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    const vk::raii::Instance& GetInstance() const { return instance; }
    const vk::raii::SurfaceKHR& GetSurface() const { return surface; }
    const vk::raii::PhysicalDevice& GetPhysicalDevice() const { return physicalDevice; }
    const vk::raii::Device& GetDevice() const { return device; }
    const vk::raii::Queue& GetQueue() const { return queue; }
    std::uint32_t GetQueueFamilyIndex() const { return queueFamilyIndex; }

private:
    void CreateInstance();
    void CreateDebugMessenger();
    void PickPhysicalDevice();
    void CreateDevice();
};

} // namespace GNC
