#pragma once

#include "Component.hpp"
#include "TransformsComponent.hpp"

#include "../Entity/Entity.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace GNC {

// Orbit component
// Moves its entity around a vertical axis at a constant angular speed by
// writing the TransformComponent's position each Update(). Rotation and
// scale are left alone. Without a transform it does nothing.
class OrbitComponent : public Component {
private:
    glm::vec3 center;
    float radius;
    float angularSpeed; // radians per second; negative goes clockwise seen from above
    float angle;        // radians, 0 = +X

public:
    OrbitComponent(const glm::vec3& orbitCenter, float orbitRadius, float radiansPerSecond, float startAngle = 0.0f)
        : center(orbitCenter), radius(orbitRadius), angularSpeed(radiansPerSecond), angle(startAngle) {}

    void Update(float deltaTime) override {
        // Wrapped so the angle keeps its float precision however long it runs.
        angle = std::fmod(angle + angularSpeed * deltaTime, glm::two_pi<float>());

        TransformComponent* transform = owner->GetComponent<TransformComponent>();
        if (!transform) return;

        transform->SetPosition(center + glm::vec3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius));
    }
};

} // namespace GNC
