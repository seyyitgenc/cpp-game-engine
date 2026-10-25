#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace GNC {

class Texture;

enum class AlphaMode : std::uint32_t {
    Opaque,
    // Cut out where the base color's alpha is below alphaCutoff.
    Mask,
    // Drawn as Opaque: there is no forward pass for transparency yet.
    Blend
};

// Material
// glTF metallic-roughness: factors, multiplied by the texture maps where
// there are any. Defaults are glTF's, which makes an untouched material a
// white, fully rough metal. Colors are linear: convert a color picked in sRGB
// first. The textures belong to the ResourceManager.
struct Material {
    glm::vec4 baseColorFactor {1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    glm::vec3 emissiveFactor {0.0f};
    // KHR_materials_emissive_strength: scales emissiveFactor past 1 for HDR
    // emitters (glTF caps the factor itself at 1).
    float emissiveStrength = 1.0f;

    Texture* baseColorTexture = nullptr;
    // glTF's channels: g roughness, b metallic.
    Texture* metallicRoughnessTexture = nullptr;
    Texture* normalTexture = nullptr;
    float normalScale = 1.0f;
    // r, applied to ambient light only.
    Texture* occlusionTexture = nullptr;
    float occlusionStrength = 1.0f;
    Texture* emissiveTexture = nullptr;

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    // Not culled, and back faces are lit from their own side.
    bool doubleSided = false;
};

} // namespace GNC
