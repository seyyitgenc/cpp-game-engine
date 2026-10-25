#include "VulkanContext.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Platform/Window.hpp"
#include "Utils/Log.hpp"

namespace GNC {

namespace {

#ifdef NDEBUG
constexpr bool EnableValidation = false;
#else
constexpr bool EnableValidation = true;
#endif

constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

// Dynamic rendering and synchronization2 are core in 1.3, so the swapchain is
// the only extension left to ask for.
constexpr std::array DeviceExtensions { vk::KHRSwapchainExtensionName };

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                               vk::DebugUtilsMessageTypeFlagsEXT,
                                               const vk::DebugUtilsMessengerCallbackDataEXT* data, void*) {
    // Not writeFormatted: its 256 character buffer truncates validation
    // messages.
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        Log::write(Log::Error, RED_TEXT("[Vulkan] "), data->pMessage, "\n");
    } else {
        Log::write(Log::Warning, YELLOW_TEXT("[Vulkan] "), data->pMessage, "\n");
    }
    // Never abort the call that triggered the message.
    return vk::False;
}

bool HasExtension(const std::vector<vk::ExtensionProperties>& available, const char* name) {
    return std::ranges::any_of(available, [name](const vk::ExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}

// A queue family that can both draw and present to the surface, if any.
std::optional<std::uint32_t> FindGraphicsPresentFamily(const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface) {
    const std::vector<vk::QueueFamilyProperties> families = physicalDevice.getQueueFamilyProperties();
    for (std::uint32_t i = 0; i < families.size(); ++i) {
        if ((families[i].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(i, *surface)) {
            return i;
        }
    }
    return std::nullopt;
}

bool IsSuitable(const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface) {
    if (physicalDevice.getProperties().apiVersion < vk::ApiVersion13) return false;

    const std::vector<vk::ExtensionProperties> extensions = physicalDevice.enumerateDeviceExtensionProperties();
    if (!std::ranges::all_of(DeviceExtensions, [&](const char* name) { return HasExtension(extensions, name); })) return false;

    const auto features = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();
    const auto& vulkan13 = features.get<vk::PhysicalDeviceVulkan13Features>();
    if (!vulkan13.dynamicRendering || !vulkan13.synchronization2) return false;

    return FindGraphicsPresentFamily(physicalDevice, surface).has_value();
}

} // namespace

VulkanContext::VulkanContext(const Window& window) {
    CreateInstance();
    CreateDebugMessenger();
    // Before picking a GPU: whether it can present to this surface is part of
    // whether it is suitable.
    surface = window.CreateSurface(instance);
    PickPhysicalDevice();
    CreateDevice();
}

void VulkanContext::CreateInstance() {
    const vk::ApplicationInfo appInfo {
        .pApplicationName = "GNC Engine",
        .applicationVersion = vk::makeApiVersion(0, 1, 0, 0),
        .pEngineName = "GNC Engine",
        .engineVersion = vk::makeApiVersion(0, 1, 0, 0),
        .apiVersion = vk::ApiVersion13
    };

    std::vector<const char*> extensions = Window::GetRequiredInstanceExtensions();
    std::vector<const char*> layers;

    if constexpr (EnableValidation) {
        const std::vector<vk::LayerProperties> available = context.enumerateInstanceLayerProperties();
        const bool hasValidation = std::ranges::any_of(available, [](const vk::LayerProperties& layer) {
            return std::strcmp(layer.layerName, ValidationLayer) == 0;
        });
        // Not fatal: it only means no safety net. It ships with the Vulkan SDK.
        if (hasValidation) {
            layers.push_back(ValidationLayer);
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
            validationEnabled = true;
        } else {
            Log::write(Log::Warning, YELLOW_TEXT("[Vulkan] "), ValidationLayer, " not found, running without validation\n");
        }
    }

    const std::vector<vk::ExtensionProperties> availableExtensions = context.enumerateInstanceExtensionProperties();
    for (const char* name : extensions) {
        if (!HasExtension(availableExtensions, name)) {
            throw std::runtime_error(std::string("VulkanContext: instance extension not supported: ") + name);
        }
    }

    instance = vk::raii::Instance(context, vk::InstanceCreateInfo {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    });
}

void VulkanContext::CreateDebugMessenger() {
    if (!validationEnabled) return;

    debugMessenger = vk::raii::DebugUtilsMessengerEXT(instance, vk::DebugUtilsMessengerCreateInfoEXT {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                     | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &DebugCallback
    });
}

void VulkanContext::PickPhysicalDevice() {
    const vk::raii::PhysicalDevices candidates(instance);

    // First suitable discrete GPU, else the first suitable anything (an
    // integrated GPU still beats not starting).
    const vk::raii::PhysicalDevice* chosen = nullptr;
    for (const vk::raii::PhysicalDevice& candidate : candidates) {
        if (!IsSuitable(candidate, surface)) continue;

        if (candidate.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            chosen = &candidate;
            break;
        }
        if (!chosen) chosen = &candidate;
    }
    if (!chosen) {
        throw std::runtime_error("VulkanContext: no GPU with Vulkan 1.3, dynamic rendering, synchronization2 and presentation support");
    }

    physicalDevice = *chosen;
    queueFamilyIndex = *FindGraphicsPresentFamily(physicalDevice, surface);

    Log::writeFormatted(Log::Info, "[Vulkan] using %s", physicalDevice.getProperties().deviceName.data());
}

void VulkanContext::CreateDevice() {
    const float queuePriority = 1.0f;
    const vk::DeviceQueueCreateInfo queueInfo {
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    const vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features> features {
        {},
        { .synchronization2 = vk::True, .dynamicRendering = vk::True }
    };

    // Setters, not designated initializers: the deprecated layer fields sit
    // mid-struct without defaults, so naming them warns and skipping them
    // warns too.
    vk::DeviceCreateInfo createInfo {};
    createInfo.setPNext(&features.get<vk::PhysicalDeviceFeatures2>())
        .setQueueCreateInfos(queueInfo)
        .setPEnabledExtensionNames(DeviceExtensions);
    device = vk::raii::Device(physicalDevice, createInfo);

    queue = vk::raii::Queue(device, queueFamilyIndex, 0);
}

} // namespace GNC
