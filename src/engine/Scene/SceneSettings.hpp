#pragma once

#include <glm/glm.hpp>

namespace GNC {

// Both kinds share entities, components and the file layout; the dimension
// only says how the editor treats the scene (docs/SceneFormat.md).
enum class SceneDimension {
    TwoD,
    ThreeD
};

// Scene-wide render settings, which App hands the Renderer every frame.
struct SceneEnvironment {
    // Linear RGB, already multiplied by its intensity: the flat ambient term.
    glm::vec3 ambientColor {0.03f};
    float exposure = 1.0f;
};

} // namespace GNC
