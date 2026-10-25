#pragma once

#include <glm/glm.hpp>

namespace GNC {

// Material
// glTF metallic-roughness factors; texture maps come with texture loading.
// Defaults are glTF's, which makes an untouched material a white, fully rough
// metal. Colors are linear: convert a color picked in sRGB first.
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
