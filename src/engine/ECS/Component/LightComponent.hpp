#pragma once

#include "Component.hpp"

#include <algorithm>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace GNC {

// Values match LIGHT_TYPE_* in shaders/common/frame.slang.
enum class LightType : std::uint32_t {
    Directional = 0,
    Point = 1,
    Spot = 2
};

// Light component
// A punctual light, modelled on glTF's KHR_lights_punctual so lights imported
// from glTF map over one to one. Where the light is and which way it points
// come from the entity's TransformComponent: it shines down its local -Z axis.
// Without a transform it sits at the origin, pointing down world -Z.
//
// Units are physical, as in glTF:
//   - point and spot: luminous intensity, in candela (lm/sr)
//   - directional:    illuminance, in lux (lm/m^2)
// Realistic values therefore span orders of magnitude (a 60W bulb is ~70 cd,
// direct sunlight ~100000 lux). Exposure in post-processing is what maps them
// back onto the screen.
class LightComponent : public Component {
private:
    LightType type = LightType::Point;
    glm::vec3 color {1.0f}; // linear RGB
    float intensity = 1.0f;

    // Point and spot only: the distance at which the light has faded to zero.
    // 0 means no cutoff, just inverse square falloff.
    float range = 0.0f;

    // Spot only, in radians, measured from the axis. Full intensity inside the
    // inner cone, none outside the outer one. glTF's defaults.
    float innerConeAngle = 0.0f;
    float outerConeAngle = glm::quarter_pi<float>();

public:
    explicit LightComponent(LightType lightType = LightType::Point) : type(lightType) {}

    void SetType(LightType lightType) { type = lightType; }
    void SetColor(const glm::vec3& linearColor) { color = linearColor; }
    void SetIntensity(float value) { intensity = std::max(value, 0.0f); }
    void SetRange(float distance) { range = std::max(distance, 0.0f); }

    // glTF's constraints: 0 <= inner < outer <= pi/2.
    void SetSpotCone(float innerRadians, float outerRadians) {
        outerConeAngle = std::clamp(outerRadians, 0.0f, glm::half_pi<float>());
        innerConeAngle = std::clamp(innerRadians, 0.0f, outerConeAngle);
    }

    LightType GetType() const { return type; }
    const glm::vec3& GetColor() const { return color; }
    float GetIntensity() const { return intensity; }
    float GetRange() const { return range; }
    float GetInnerConeAngle() const { return innerConeAngle; }
    float GetOuterConeAngle() const { return outerConeAngle; }
};

} // namespace GNC
