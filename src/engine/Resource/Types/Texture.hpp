#pragma once

#include "../Resource.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "../../Renderer/TextureTable.hpp"

namespace GNC {

// What a glTF sampler says, plus the color space of the texture's role.
struct TextureSettings {
    // Base color and emissive maps are sRGB; data maps are linear.
    bool srgb = true;
    vk::SamplerAddressMode wrapU = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode wrapV = vk::SamplerAddressMode::eRepeat;
    vk::Filter magFilter = vk::Filter::eLinear;
    vk::Filter minFilter = vk::Filter::eLinear;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
    bool mipmaps = true;
};

// An encoded image stb_image reads (PNG, JPEG, ...): a file, or its bytes
// (embedded in a model file).
using TextureSource = std::variant<std::filesystem::path, std::vector<std::uint8_t>>;

// Where an image's bytes came from, so a scene file can name it: an image
// embedded in a model file (model relative to the executable), by the name
// the file gives it ("*0" for glTF's, the file name for FBX's).
struct EmbeddedImage {
    std::string model;
    std::string name;
};

// Texture resource
// Decoded to RGBA8, uploaded with a full mip chain, and added to the
// ResourceContext's TextureTable, whose slot materials use.
class Texture : public Resource {
private:
    TextureSource source;
    TextureSettings settings;
    // Only for bytes: where they came from.
    std::optional<EmbeddedImage> embedded;

    vk::raii::DeviceMemory memory = nullptr;
    vk::raii::Image image = nullptr;
    vk::raii::ImageView imageView = nullptr;
    vk::raii::Sampler sampler = nullptr;

    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t mipLevels = 0;
    std::uint32_t tableSlot = TextureTable::White;

public:
    Texture(const std::string& id, TextureSource textureSource, TextureSettings textureSettings,
            std::optional<EmbeddedImage> embeddedImage = std::nullopt)
        : Resource(id), source(std::move(textureSource)), settings(textureSettings), embedded(std::move(embeddedImage)) {}

    // TextureTable::White until loaded.
    std::uint32_t GetTableSlot() const { return tableSlot; }

    std::uint32_t GetWidth() const { return width; }
    std::uint32_t GetHeight() const { return height; }
    const TextureSettings& GetSettings() const { return settings; }
    const TextureSource& GetSource() const { return source; }
    const std::optional<EmbeddedImage>& GetEmbeddedImage() const { return embedded; }

    std::vector<std::string> GetSourcePaths() const override;

protected:
    bool doLoad() override;
    void doUnload() override;

private:
    void CreateSampler();
};

} // namespace GNC
