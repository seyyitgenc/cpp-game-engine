#pragma once

#include "io/keyboard.h"
#include "io/mouse.h"

#include <glm/glm.hpp>
#include <vector>

// camera movement directions
enum CameraDirection {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// default camera values
constexpr float YAW = -90.0f;
constexpr float PITCH = 0.0f;
constexpr float SPEED = 30.0f;
constexpr float SENSITIVITY = 0.1f;
constexpr float ZOOM = 45.0f;

class Camera {
public:
    glm::vec3 Position {};
    glm::vec3 Front {};
    glm::vec3 Up {};
    glm::vec3 Right {};
    glm::vec3 WorldUp {};

    glm::mat4 Projection {};

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    void setProjectionMatrixAsOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    void setProjectionMatrixAsPerspective(float fovY, float aspectRatio, float nearPlane, float farPlane);

    glm::mat4 GetViewMatrix();

    void updateCameraPosition(CameraDirection direction, float deltaTime);

    void updateCameraDirection(double dx, double dy);
    void updateCameraZoom(double dy);

    void handleEvents(float dt);

private:
    void updateCameraVectors();
};