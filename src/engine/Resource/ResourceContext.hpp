#pragma once

#include <vulkan/vulkan_raii.hpp>

// Resource context
// The manager holds one context and hands a pointer to it to every
// resource it creates, so nothing here has to know about a global device.
//
// Non-owning. Whoever owns the device must outlive the ResourceManager.
struct ResourceContext {
    const vk::raii::Device* device = nullptr;
    const vk::raii::PhysicalDevice* physicalDevice = nullptr;

    // Used by the staging uploads that fill textures and vertex buffers.
    const vk::raii::Queue* transferQueue = nullptr;
    const vk::raii::CommandPool* transferPool = nullptr;

    bool IsValid() const { return device != nullptr && physicalDevice != nullptr; }
};
