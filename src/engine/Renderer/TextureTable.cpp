#include "TextureTable.hpp"

#include "Utils/VulkanHelpers.hpp"

namespace GNC {

TextureTable::TextureTable(const ResourceContext& context) : device(*context.device) {
    white = CreateDefault(context, { 255, 255, 255, 255 });
    // (0, 0, 1) in tangent space: the surface normal itself.
    flatNormal = CreateDefault(context, { 128, 128, 255, 255 });
    defaultSampler = vk::raii::Sampler(device, vk::SamplerCreateInfo {
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest
    });

    const vk::DescriptorSetLayoutBinding binding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = Capacity,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };
    layout = vk::raii::DescriptorSetLayout(device, vk::DescriptorSetLayoutCreateInfo {
        .bindingCount = 1,
        .pBindings = &binding
    });

    const vk::DescriptorPoolSize poolSize {
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = Capacity
    };
    // eFreeDescriptorSet: a vk::raii::DescriptorSet frees itself into its pool.
    pool = vk::raii::DescriptorPool(device, vk::DescriptorPoolCreateInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    });

    const vk::DescriptorSetLayout setLayout = *layout;
    vk::raii::DescriptorSets sets(device, vk::DescriptorSetAllocateInfo {
        .descriptorPool = *pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &setLayout
    });
    set = std::move(sets.front());

    std::vector<vk::DescriptorImageInfo> imageInfos(Capacity, vk::DescriptorImageInfo {
        .sampler = *defaultSampler,
        .imageView = *white.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    });
    imageInfos[FlatNormal].imageView = *flatNormal.view;
    device.updateDescriptorSets(vk::WriteDescriptorSet {
        .dstSet = *set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = Capacity,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = imageInfos.data()
    }, {});

    // Handed out from the low end.
    for (std::uint32_t slot = Capacity - 1; slot > FlatNormal; --slot) {
        freeSlots.push_back(slot);
    }
}

std::optional<std::uint32_t> TextureTable::Add(vk::ImageView view, vk::Sampler sampler) {
    if (freeSlots.empty()) return std::nullopt;

    const std::uint32_t slot = freeSlots.back();
    freeSlots.pop_back();
    Write(slot, view, sampler);
    return slot;
}

void TextureTable::Remove(std::uint32_t slot) {
    if (slot <= FlatNormal || slot >= Capacity) return;

    Write(slot, *white.view, *defaultSampler);
    freeSlots.push_back(slot);
}

TextureTable::DefaultTexture TextureTable::CreateDefault(const ResourceContext& context, const std::array<std::uint8_t, 4>& rgba) {
    constexpr vk::Format Format = vk::Format::eR8G8B8A8Unorm;

    DefaultTexture texture;
    uploadImage(*context.device, *context.physicalDevice, *context.transferQueue, *context.transferPool,
        rgba.data(), 1, 1, Format, 1, texture.image, texture.memory);
    texture.view = createImageView(*context.device, texture.image, Format);
    return texture;
}

void TextureTable::Write(std::uint32_t slot, vk::ImageView view, vk::Sampler sampler) {
    const vk::DescriptorImageInfo imageInfo {
        .sampler = sampler,
        .imageView = view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    device.updateDescriptorSets(vk::WriteDescriptorSet {
        .dstSet = *set,
        .dstBinding = 0,
        .dstArrayElement = slot,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo
    }, {});
}

} // namespace GNC
