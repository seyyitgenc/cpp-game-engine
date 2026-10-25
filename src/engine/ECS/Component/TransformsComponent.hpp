#pragma once

#include "Component.hpp"
#include "Property.hpp"

#include "../Entity/Entity.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace GNC {

// Transform component
// Position, rotation and scale relative to the nearest ancestor that has a
// transform (the world, if none does).
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

    // Split into position, rotation and scale; shear is lost, and a
    // mirroring matrix comes back as a negative x scale.
    void SetLocalMatrix(const glm::mat4& matrix) {
        glm::vec3 axes[3] = { glm::vec3(matrix[0]), glm::vec3(matrix[1]), glm::vec3(matrix[2]) };
        glm::vec3 lengths(glm::length(axes[0]), glm::length(axes[1]), glm::length(axes[2]));
        if (glm::dot(glm::cross(axes[0], axes[1]), axes[2]) < 0.0f) lengths.x = -lengths.x;

        position = glm::vec3(matrix[3]);
        scale = lengths;
        // A zero scale leaves no axis to take the rotation from.
        if (lengths.x != 0.0f && lengths.y != 0.0f && lengths.z != 0.0f) {
            rotation = glm::normalize(glm::quat_cast(glm::mat3 { axes[0] / lengths.x, axes[1] / lengths.y, axes[2] / lengths.z }));
        }
        transformDirty = true;
    }

    const glm::vec3& GetPosition() const { return position; }
    const glm::quat& GetRotation() const { return rotation; }
    const glm::vec3& GetScale() const { return scale; }

    glm::mat4 GetLocalMatrix() const {
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

    // Not cached: each call walks up through the ancestors.
    glm::mat4 GetParentMatrix() const {
        for (Entity* ancestor = owner ? owner->GetParent() : nullptr; ancestor; ancestor = ancestor->GetParent()) {
            if (const TransformComponent* transform = ancestor->GetComponent<TransformComponent>()) return transform->GetWorldMatrix();
        }
        return glm::mat4(1.0f);
    }

    glm::mat4 GetWorldMatrix() const { return GetParentMatrix() * GetLocalMatrix(); }

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