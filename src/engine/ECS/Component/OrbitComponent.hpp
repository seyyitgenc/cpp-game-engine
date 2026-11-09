#pragma once

#include "Component.hpp"
#include "Property.hpp"
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
    glm::vec3 center {0.0f};
    float radius = 1.0f;
    float angularSpeed = 0.0f; // radians per second; negative goes clockwise seen from above
    float angle = 0.0f;        // radians, 0 = +X

public:
    GNC_COMPONENT_TYPE(OrbitComponent, "Orbit", "Orbit")

    OrbitComponent() = default;
    OrbitComponent(const glm::vec3& orbitCenter, float orbitRadius, float radiansPerSecond, float startAngle = 0.0f)
        : center(orbitCenter), radius(orbitRadius), angularSpeed(radiansPerSecond), angle(startAngle) {}

    void SetCenter(const glm::vec3& orbitCenter) { center = orbitCenter; }
    void SetRadius(float orbitRadius) { radius = orbitRadius; }
    void SetAngularSpeed(float radiansPerSecond) { angularSpeed = radiansPerSecond; }
    void SetAngle(float radians) { angle = std::fmod(radians, glm::two_pi<float>()); }

    const glm::vec3& GetCenter() const { return center; }
    float GetRadius() const { return radius; }
    float GetAngularSpeed() const { return angularSpeed; }
    float GetAngle() const { return angle; }

    void Update(float deltaTime) override {
        // Wrapped so the angle keeps its float precision however long it runs.
        angle = std::fmod(angle + angularSpeed * deltaTime, glm::two_pi<float>());

        TransformComponent* transform = owner->GetComponent<TransformComponent>();
        if (!transform) return;

        transform->SetPosition(center + glm::vec3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius));
    }

    static void Reflect(PropertyList<OrbitComponent>& properties) {
        properties.Add<glm::vec3>(
            { .name = "Center", .key = "center", .step = 0.05f },
            [](const OrbitComponent& orbit) { return orbit.GetCenter(); },
            [](OrbitComponent& orbit, const glm::vec3& value) { orbit.SetCenter(value); });

        properties.Add<float>(
            { .name = "Radius", .key = "radius", .min = 0.0f, .step = 0.05f, .format = "%.2f" },
            [](const OrbitComponent& orbit) { return orbit.GetRadius(); },
            [](OrbitComponent& orbit, float value) { orbit.SetRadius(value); });

        properties.Add<float>(
            { .name = "Speed", .key = "angularSpeed", .hint = PropertyHint::Angle, .step = 1.0f, .format = "%.0f°/s" },
            [](const OrbitComponent& orbit) { return orbit.GetAngularSpeed(); },
            [](OrbitComponent& orbit, float value) { orbit.SetAngularSpeed(value); });

        // Where it is on the circle now; a scene file saves the phase with it.
        properties.Add<float>(
            { .name = "Angle", .key = "angle", .hint = PropertyHint::Angle },
            [](const OrbitComponent& orbit) { return orbit.GetAngle(); },
            [](OrbitComponent& orbit, float value) { orbit.SetAngle(value); });
    }
};

} // namespace GNC
