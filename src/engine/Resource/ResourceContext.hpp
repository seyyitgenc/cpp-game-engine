#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

class TextureTable;

// Resource context
// Handed to every resource the manager creates. Non-owning: the device's
// owner must outlive the ResourceManager.
struct ResourceContext {
    const vk::raii::Device* device = nullptr;
    const vk::raii::PhysicalDevice* physicalDevice = nullptr;

    // For staging uploads.
    const vk::raii::Queue* transferQueue = nullptr;
    const vk::raii::CommandPool* transferPool = nullptr;

    // Where textures make themselves visible to shaders.
    TextureTable* textures = nullptr;

    bool IsValid() const { return device != nullptr && physicalDevice != nullptr; }
};

} // namespace GNC
