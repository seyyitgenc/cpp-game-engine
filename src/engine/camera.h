#pragma once

#include "../external/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "io/keyboard.h"
#include "io/mouse.h"

// camera movement directions
enum CameraDirection{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// default camera values
constexpr float YAW          =  -90.0f;
constexpr float PITCH        =   0.0f;
constexpr float SPEED        =   30.0f;
constexpr float SENSITIVITY  =   0.1f;
constexpr float ZOOM         =   45.0f;

class Camera
{
public:
    glm::vec3 Position{};
    glm::vec3 Front;
    glm::vec3 Up{};
    glm::vec3 Right{};
    glm::vec3 WorldUp{};

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

public:
    Camera(glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f), glm::vec3 up = glm::vec3(0.0f,1.0f,0.0f), float yaw = YAW, float pitch = PITCH) :
    Position(position), Front(glm::vec3(0.0f,0.0f,-1.0f)), WorldUp(up), Yaw(yaw), Pitch(pitch), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM){
        updateCameraVectors();
    };

    Camera(float posX, float posY, float posZ, float upX ,float upY,float upZ, float yaw, float pitch) :
    Front(glm::vec3(0.0f,0.0f,-1.0f)), Yaw(yaw), Pitch(pitch), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position =  glm::vec3(posX,posY,posZ);
        WorldUp  =  glm::vec3(upX,upY,upZ);
        updateCameraVectors();
    }
    
    glm::mat4 GetViewMatrix(){
        return glm::lookAt(Position, Position + Front, Up);
    }

    void updateCameraPosition(CameraDirection direction, float deltaTime){
        float velocity = MovementSpeed * deltaTime;

        switch (direction)
        {
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

    void updateCameraDirection(double dx, double dy){
        Yaw += MouseSensitivity * dx;
        Pitch += MouseSensitivity * dy;

        if (Pitch > 89.0f)  Pitch =  89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
        updateCameraVectors();
    }

    void updateCameraZoom(double dy){
        Zoom -= (float)dy;
        if(Zoom < 1.0f) Zoom = 1.0f;
        if(Zoom > 45.0f) Zoom = 45.0f;
    }
    
    void handleEvents(float dt){
        if (Keyboard::key(GLFW_KEY_W)){
            updateCameraPosition(FORWARD, dt);
        }
        if (Keyboard::key(GLFW_KEY_S)){
            updateCameraPosition(BACKWARD, dt);
        }
        if (Keyboard::key(GLFW_KEY_A)){
            updateCameraPosition(LEFT, dt);
        }
        if (Keyboard::key(GLFW_KEY_D)){
            updateCameraPosition(RIGHT, dt);
        }

        double dx = Mouse::getDX(), dy = Mouse::getDY();
        if (dx != 0 || dy != 0)
            updateCameraDirection(dx, dy);

        double scrollDy = Mouse::getScrollDY();
        if (scrollDy != 0)
            updateCameraZoom(scrollDy);
    }
private:
    void updateCameraVectors(){
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};