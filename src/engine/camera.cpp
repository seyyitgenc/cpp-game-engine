#include "camera.h"

#include "io/gamepad.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Position(position)
    , Front(glm::vec3(0.0f, 0.0f, -1.0f))
    , WorldUp(up)
    , Yaw(yaw)
    , Pitch(pitch)
    , MovementSpeed(SPEED)
    , MouseSensitivity(SENSITIVITY)
    , Zoom(ZOOM)
    , Projection(glm::mat4(1.0f))
{
    updateCameraVectors();
};

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f))
    , Yaw(yaw)
    , Pitch(pitch)
    , MovementSpeed(SPEED)
    , MouseSensitivity(SENSITIVITY)
    , Zoom(ZOOM)
    , Projection(glm::mat4(1.0f))
{
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    updateCameraVectors();
}

void Camera::updateCameraPosition(CameraDirection direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;

    switch (direction) {
    case FORWARD:
        Position += Front * velocity;
        break;
    case BACKWARD:
        Position -= Front * velocity;
        break;
    case RIGHT:
        Position += Right * velocity;
        break;
    case LEFT:
        Position -= Right * velocity;
        break;
    case UP:
        Position += Up * velocity;
        break;
    case DOWN:
        Position -= Right * velocity;
        break;
    }
}

void Camera::updateCameraDirection(double dx, double dy)
{
    Yaw += MouseSensitivity * dx;
    Pitch += MouseSensitivity * dy;

    if (Pitch > 89.0f)
        Pitch = 89.0f;
    if (Pitch < -89.0f)
        Pitch = -89.0f;
    updateCameraVectors();
}

void Camera::updateCameraZoom(double dy)
{
    Zoom -= (float)dy;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::handleEvents(float dt)
{
    if (Keyboard::key(GLFW_KEY_W) || Gamepad::isButtonPushed(GLFW_JOYSTICK_1, 12)) {
        updateCameraPosition(FORWARD, dt);
    }
    if (Keyboard::key(GLFW_KEY_S) || Gamepad::isButtonPushed(GLFW_JOYSTICK_1, 14)) {
        updateCameraPosition(BACKWARD, dt);
    }
    if (Keyboard::key(GLFW_KEY_A) || Gamepad::isButtonPushed(GLFW_JOYSTICK_1, 15)) {
        updateCameraPosition(LEFT, dt);
    }
    if (Keyboard::key(GLFW_KEY_D) || Gamepad::isButtonPushed(GLFW_JOYSTICK_1, 13)) {
        updateCameraPosition(RIGHT, dt);
    }

    double dx = Mouse::getDX(), dy = Mouse::getDY();
    if (dx != 0 || dy != 0)
        updateCameraDirection(dx, dy);

    double scrollDy = Mouse::getScrollDY();
    if (scrollDy != 0)
        updateCameraZoom(scrollDy);
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::setProjectionMatrixAsOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    Projection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

void Camera::setProjectionMatrixAsPerspective(float fovY, float aspectRatio, float nearPlane, float farPlane)
{
    Projection = glm::perspective(glm::radians(fovY), aspectRatio, nearPlane, farPlane);
}

glm::mat4 Camera::GetViewMatrix() { return glm::lookAt(Position, Position + Front, Up); }
