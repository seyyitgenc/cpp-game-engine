#pragma once

#include <glm/glm.hpp>

namespace GNC {

// Material
// Surface description for the glTF metallic-roughness model. For now only the
// constant factors; the texture maps (base color, metallic-roughness, normal,
// occlusion, emissive) multiply into these once texture loading exists.
//
// Defaults are glTF's, so an importer can leave out whatever the file leaves
// out. Note that this makes an untouched material a white, fully rough metal.
//
// Colors are linear. A color picked in sRGB (most color pickers, hex codes)
// has to be converted first, or it will come out too bright.
struct Material {
    glm::vec4 baseColorFactor {1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    glm::vec3 emissiveFactor {0.0f};
    // KHR_materials_emissive_strength: scales emissiveFactor past 1 for HDR
    // emitters (glTF caps the factor itself at 1).
    float emissiveStrength = 1.0f;
};

} // namespace GNC
