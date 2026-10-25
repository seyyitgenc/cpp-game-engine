#include "Texture.hpp"

#include <algorithm>
#include <bit>
#include <limits>

#include "stb_image.h"

#include "../../Utils/Log.hpp"
#include "../../Utils/VulkanHelpers.hpp"

namespace GNC {

std::vector<std::string> Texture::GetSourcePaths() const {
    if (const auto* path = std::get_if<std::filesystem::path>(&source)) return { path->string() };
    return {};
}

bool Texture::doLoad() {
    const ResourceContext& context = GetContext();
    if (!context.transferQueue || !context.transferPool || !context.textures) return false;

    int imageWidth = 0;
    int imageHeight = 0;
    int channels = 0;
    stbi_uc* texels = nullptr;
    if (const auto* path = std::get_if<std::filesystem::path>(&source)) {
        texels = stbi_load(path->string().c_str(), &imageWidth, &imageHeight, &channels, STBI_rgb_alpha);
    } else {
        const auto& bytes = std::get<std::vector<std::uint8_t>>(source);
        if (bytes.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            texels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &imageWidth, &imageHeight, &channels, STBI_rgb_alpha);
        }
    }
    if (!texels) {
        Log::write(Log::Error, "Texture '", GetId(), "': ", stbi_failure_reason(), "\n");
        return false;
    }

    width = static_cast<std::uint32_t>(imageWidth);
    height = static_cast<std::uint32_t>(imageHeight);
    mipLevels = settings.mipmaps ? static_cast<std::uint32_t>(std::bit_width(std::max(width, height))) : 1;

    const vk::Format format = settings.srgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
    uploadImage(GetDevice(), *context.physicalDevice, *context.transferQueue, *context.transferPool,
        texels, width, height, format, mipLevels, image, memory);
    stbi_image_free(texels);

    imageView = createImageView(GetDevice(), image, format, vk::ImageAspectFlagBits::eColor, mipLevels);
    CreateSampler();

    // uploadImage() waited for the queue, so no frame uses the table now.
    const std::optional<std::uint32_t> slot = context.textures->Add(*imageView, *sampler);
    if (!slot) {
        Log::write(Log::Error, "Texture '", GetId(), "': the texture table is full (", TextureTable::Capacity, ")\n");
        doUnload();
        return false;
    }
    tableSlot = *slot;
    return true;
}

void Texture::doUnload() {
    if (tableSlot != TextureTable::White) GetContext().textures->Remove(tableSlot);
    tableSlot = TextureTable::White;

    // The view sits on the image, the image in the memory.
    sampler = nullptr;
    imageView = nullptr;
    image = nullptr;
    memory = nullptr;

    width = height = mipLevels = 0;
}

void Texture::CreateSampler() {
    const float maxAnisotropy = GetContext().physicalDevice->getProperties().limits.maxSamplerAnisotropy;
    sampler = vk::raii::Sampler(GetDevice(), vk::SamplerCreateInfo {
        .magFilter = settings.magFilter,
        .minFilter = settings.minFilter,
        .mipmapMode = settings.mipmapMode,
        .addressModeU = settings.wrapU,
        .addressModeV = settings.wrapV,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = std::min(16.0f, maxAnisotropy),
        .maxLod = static_cast<float>(mipLevels)
    });
}

} // namespace GNC
