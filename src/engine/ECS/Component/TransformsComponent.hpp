#pragma once

#include "Component.hpp"
#include "Property.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace GNC {

// Transform component
// Handles the position, rotation, and scale of an entity in 3D space
// AffineTransform or "Pose" matrix.
class TransformComponent : public Component {
private:
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
    glm::vec3 scale = glm::vec3(1.0f);

    // Cached transformation matrix
    mutable glm::mat4 transformMatrix = glm::mat4(1.0f);
    mutable bool transformDirty = true;

public:
    GNC_COMPONENT_TYPE(TransformComponent, "Transform")

    void SetPosition(const glm::vec3& pos) {
        position = pos;
        transformDirty = true;
    }

    void SetRotation(const glm::quat& rot) {
        rotation = rot;
        transformDirty = true;
    }

    void SetScale(const glm::vec3& s) {
        scale = s;
        transformDirty = true;
    }

    const glm::vec3& GetPosition() const { return position; }
    const glm::quat& GetRotation() const { return rotation; }
    const glm::vec3& GetScale() const { return scale; }

    glm::mat4 GetTransformMatrix() const {
        if (transformDirty) {
            // Calculate transformation matrix
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
            glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

            transformMatrix = translationMatrix * rotationMatrix * scaleMatrix;
            transformDirty = false;
        }
        return transformMatrix;
    }

    static void Reflect(PropertyList<TransformComponent>& properties) {
        properties.Add<glm::vec3>(
            { .name = "Position", .step = 0.05f },
            [](const TransformComponent& transform) { return transform.GetPosition(); },
            [](TransformComponent& transform, const glm::vec3& value) { transform.SetPosition(value); });

        properties.Add<glm::quat>(
            { .name = "Rotation" },
            [](const TransformComponent& transform) { return transform.GetRotation(); },
            [](TransformComponent& transform, const glm::quat& value) { transform.SetRotation(value); });

        // Not 0: that flattens the model matrix, and lights normalize their
        // local -Z axis.
        properties.Add<glm::vec3>(
            { .name = "Scale", .min = 0.001f, .step = 0.01f },
            [](const TransformComponent& transform) { return transform.GetScale(); },
            [](TransformComponent& transform, const glm::vec3& value) { transform.SetScale(value); });
    }
};

} // namespace GNC