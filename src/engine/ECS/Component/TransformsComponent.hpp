#pragma once

#include "Component.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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
};