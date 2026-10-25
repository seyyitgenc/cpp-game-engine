#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace GNC {

// Relative to the camera: Up/Down follow its up vector, not the world's.
enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

enum class ProjectionType {
    Perspective,
    Orthographic
};

// std140; the shader side is CameraData in shaders/common/frame.slang.
// position is a vec4 so both sides have the same size.
struct CameraUniform {
    glm::mat4 view {1.0f};
    glm::mat4 projection {1.0f};
    glm::mat4 viewProjection {1.0f};
    // The lighting pass rebuilds world positions from depth with it.
    glm::mat4 inverseViewProjection {1.0f};
    glm::vec4 position {0.0f, 0.0f, 0.0f, 1.0f};
};
static_assert(sizeof(CameraUniform) == 272);

// Camera
// Free-look camera driven by yaw and pitch, without roll; yaw -90, pitch 0
// looks down -Z. The projection is Vulkan's: 0..1 depth, and Y flipped here
// rather than in the UBO, so every user of GetProjectionMatrix() (shaders,
// culling) sees the same matrix. Its owner applies input through Process*().

// TODO: implement third person camera
class Camera {
public:
    static constexpr float DefaultYaw = -90.0f;
    static constexpr float DefaultPitch = 0.0f;
    static constexpr float DefaultSpeed = 5.0f;
    static constexpr float DefaultSensitivity = 0.1f;
    static constexpr float DefaultFov = 45.0f;

    static constexpr float MinFov = 1.0f;
    static constexpr float MaxFov = 90.0f;
    // Short of 90: there front is parallel to worldUp, and the cross product
    // for right collapses to zero.
    static constexpr float PitchLimit = 89.0f;

private:
    // ----
    // pose
    // ----
    glm::vec3 position {0.0f};
    glm::vec3 front {0.0f, 0.0f, -1.0f};
    glm::vec3 up {0.0f, 1.0f, 0.0f};
    glm::vec3 right {1.0f, 0.0f, 0.0f};
    glm::vec3 worldUp {0.0f, 1.0f, 0.0f};

    // Euler angles, in degrees
    float yaw = DefaultYaw;
    float pitch = DefaultPitch;

    // -------
    // control
    // -------
    float movementSpeed = DefaultSpeed;
    float mouseSensitivity = DefaultSensitivity;

    // ----------
    // projection
    // ----------
    ProjectionType projectionType = ProjectionType::Perspective;
    float fov = DefaultFov; // vertical, in degrees
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    // Half extents of the orthographic view volume
    float orthoHalfWidth = 10.0f;
    float orthoHalfHeight = 10.0f;

    mutable glm::mat4 viewMatrix {1.0f};
    mutable glm::mat4 projectionMatrix {1.0f};
    mutable bool viewDirty = true;
    mutable bool projectionDirty = true;

public:
    explicit Camera(const glm::vec3& pos = glm::vec3(0.0f),
                    const glm::vec3& worldUpVector = glm::vec3(0.0f, 1.0f, 0.0f),
                    float yawDegrees = DefaultYaw,
                    float pitchDegrees = DefaultPitch)
        : position(pos), worldUp(worldUpVector), yaw(yawDegrees), pitch(pitchDegrees) {
        UpdateCameraVectors();
    }

    // --------
    // matrices
    // --------
    const glm::mat4& GetViewMatrix() const {
        if (viewDirty) {
            viewMatrix = glm::lookAt(position, position + front, up);
            viewDirty = false;
        }
        return viewMatrix;
    }

    const glm::mat4& GetProjectionMatrix() const {
        if (projectionDirty) {
            if (projectionType == ProjectionType::Perspective) {
                projectionMatrix = glm::perspectiveRH_ZO(glm::radians(fov), aspectRatio, nearPlane, farPlane);
            } else {
                projectionMatrix = glm::orthoRH_ZO(-orthoHalfWidth, orthoHalfWidth,
                                                   -orthoHalfHeight, orthoHalfHeight,
                                                   nearPlane, farPlane);
            }
            projectionMatrix[1][1] *= -1.0f; // Vulkan clip space Y points down
            projectionDirty = false;
        }
        return projectionMatrix;
    }

    glm::mat4 GetViewProjectionMatrix() const {
        return GetProjectionMatrix() * GetViewMatrix();
    }

    CameraUniform GetUniform() const {
        CameraUniform uniform;
        uniform.view = GetViewMatrix();
        uniform.projection = GetProjectionMatrix();
        uniform.viewProjection = uniform.projection * uniform.view;
        uniform.inverseViewProjection = glm::inverse(uniform.viewProjection);
        uniform.position = glm::vec4(position, 1.0f);
        return uniform;
    }

    // -----
    // input
    // -----

    void ProcessKeyboard(CameraMovement direction, float deltaTime) {
        const float velocity = movementSpeed * deltaTime;
        switch (direction) {
            case CameraMovement::Forward:  position += front * velocity; break;
            case CameraMovement::Backward: position -= front * velocity; break;
            case CameraMovement::Left:     position -= right * velocity; break;
            case CameraMovement::Right:    position += right * velocity; break;
            case CameraMovement::Up:       position += up * velocity;    break;
            case CameraMovement::Down:     position -= up * velocity;    break;
        }
        viewDirty = true;
    }

    // Raw cursor deltas, yOffset as (lastY - currentY): screen Y grows
    // downwards, pitch upwards.
    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true) {
        yaw += xOffset * mouseSensitivity;
        pitch += yOffset * mouseSensitivity;

        if (constrainPitch) {
            pitch = std::clamp(pitch, -PitchLimit, PitchLimit);
        }
        UpdateCameraVectors();
    }

    void ProcessMouseScroll(float yOffset) {
        SetFov(fov - yOffset);
    }

    // ----
    // pose
    // ----
    void SetPosition(const glm::vec3& pos) {
        position = pos;
        viewDirty = true;
    }

    void SetRotation(float yawDegrees, float pitchDegrees) {
        yaw = yawDegrees;
        pitch = std::clamp(pitchDegrees, -PitchLimit, PitchLimit);
        UpdateCameraVectors();
    }

    // Through yaw and pitch, so mouse look continues from the new direction.
    void LookAt(const glm::vec3& target) {
        const glm::vec3 direction = target - position;
        if (glm::length(direction) <= 0.0f) return;

        const glm::vec3 dir = glm::normalize(direction);
        SetRotation(glm::degrees(std::atan2(dir.z, dir.x)),
                    glm::degrees(std::asin(std::clamp(dir.y, -1.0f, 1.0f))));
    }

    const glm::vec3& GetPosition() const { return position; }
    const glm::vec3& GetFront() const { return front; }
    const glm::vec3& GetUp() const { return up; }
    const glm::vec3& GetRight() const { return right; }
    float GetYaw() const { return yaw; }
    float GetPitch() const { return pitch; }

    // ----------
    // projection
    // ----------
    void SetPerspective(float fovDegrees, float aspect, float nearZ, float farZ) {
        projectionType = ProjectionType::Perspective;
        fov = std::clamp(fovDegrees, MinFov, MaxFov);
        aspectRatio = aspect;
        nearPlane = nearZ;
        farPlane = farZ;
        projectionDirty = true;
    }

    void SetOrthographic(float halfWidth, float halfHeight, float nearZ, float farZ) {
        projectionType = ProjectionType::Orthographic;
        orthoHalfWidth = halfWidth;
        orthoHalfHeight = halfHeight;
        nearPlane = nearZ;
        farPlane = farZ;
        projectionDirty = true;
    }

    void SetAspectRatio(float aspect) {
        aspectRatio = aspect;
        projectionDirty = true;
    }

    void SetFov(float fovDegrees) {
        fov = std::clamp(fovDegrees, MinFov, MaxFov);
        projectionDirty = true;
    }

    ProjectionType GetProjectionType() const { return projectionType; }
    float GetFov() const { return fov; }
    float GetAspectRatio() const { return aspectRatio; }
    float GetNearPlane() const { return nearPlane; }
    float GetFarPlane() const { return farPlane; }

    // -------
    // control
    // -------

    void SetMovementSpeed(float speed) { movementSpeed = speed; }
    void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    float GetMovementSpeed() const { return movementSpeed; }
    float GetMouseSensitivity() const { return mouseSensitivity; }

private:
    void UpdateCameraVectors() {
        const float yawRad = glm::radians(yaw);
        const float pitchRad = glm::radians(pitch);

        glm::vec3 newFront;
        newFront.x = std::cos(yawRad) * std::cos(pitchRad);
        newFront.y = std::sin(pitchRad);
        newFront.z = std::sin(yawRad) * std::cos(pitchRad);

        front = glm::normalize(newFront);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));

        viewDirty = true;
    }
};

} // namespace GNC
