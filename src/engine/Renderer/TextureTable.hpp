#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Resource/ResourceContext.hpp"

namespace GNC {

// Texture table
// Every loaded texture as one combined image sampler in a single array, set 1
// of the geometry pass, which materials index into. Free slots hold White, so
// the whole array is always valid and no descriptor indexing features are
// needed.
class TextureTable {
public:
    // shaders/geometry_pass.slang: MAX_TEXTURES
    static constexpr std::uint32_t Capacity = 1024;

    // For a material without the texture: they leave its factors unchanged.
    static constexpr std::uint32_t White = 0;
    static constexpr std::uint32_t FlatNormal = 1;

private:
    struct DefaultTexture {
        vk::raii::DeviceMemory memory = nullptr;
        vk::raii::Image image = nullptr;
        vk::raii::ImageView view = nullptr;
    };

    const vk::raii::Device& device;

    DefaultTexture white;
    DefaultTexture flatNormal;
    vk::raii::Sampler defaultSampler = nullptr;

    // The pool before the set, which frees itself into it.
    vk::raii::DescriptorSetLayout layout = nullptr;
    vk::raii::DescriptorPool pool = nullptr;
    vk::raii::DescriptorSet set = nullptr;

    std::vector<std::uint32_t> freeSlots;

public:
    // Uploads the defaults through the context's transfer queue.
    explicit TextureTable(const ResourceContext& context);

    TextureTable(const TextureTable&) = delete;
    TextureTable& operator=(const TextureTable&) = delete;

    // Both write the set, so only while no submitted frame uses it: a texture
    // adds itself right after its upload waited for the queue. Add() returns
    // nothing when the table is full.
    std::optional<std::uint32_t> Add(vk::ImageView view, vk::Sampler sampler);
    void Remove(std::uint32_t slot);

    const vk::raii::DescriptorSetLayout& GetLayout() const { return layout; }
    vk::DescriptorSet GetSet() const { return *set; }

private:
    static DefaultTexture CreateDefault(const ResourceContext& context, const std::array<std::uint8_t, 4>& rgba);
    void Write(std::uint32_t slot, vk::ImageView view, vk::Sampler sampler);
};

} // namespace GNC
