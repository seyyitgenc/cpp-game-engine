#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
// camera movement directions
enum Camera_Movement{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
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
private:    
    // fixme: due to last mouse position, camera switch works fine but it will not recognize last position
    float lastX{}, lastY{};
    bool m_firstMouse = true;

public:
    Camera(glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f), glm::vec3 up = glm::vec3(0.0f,1.0f,0.0f), float yaw = YAW, float pitch = PITCH   ) : Position(position), Front(glm::vec3(0.0f,0.0f,-1.0f)), WorldUp(up), Yaw(yaw), Pitch(pitch), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM){
        updateCameraVectors();
    };

    Camera(float posX, float posY, float posZ, float upX ,float upY,float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f,0.0f,-1.0f)), Yaw(yaw), Pitch(pitch), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position =  glm::vec3(posX,posY,posZ);
        WorldUp  =  glm::vec3(upX,upY,upZ);
        updateCameraVectors();
    }
    
    glm::mat4 GetViewMatrix(){
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(Camera_Movement direction, float deltaTime){
        float velocity = MovementSpeed * deltaTime;
        if(direction == FORWARD)
            Position += Front * velocity;
        if(direction == BACKWARD)
            Position -= Front * velocity;
        if(direction == RIGHT)
            Position += Right * velocity;
        if(direction == LEFT)
            Position -= Right * velocity;
    }

    void processMouseMovement(double xpos, double ypos){
        if (m_firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            m_firstMouse = false;
        }
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;
        Yaw   += xoffset;
        Pitch += yoffset;

        if (Pitch > 89.0f)  Pitch =  89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
        updateCameraVectors();
    }

    void processMouseScroll(double xoffset, double yoffset){
        Zoom -= (float)yoffset;
        if(Zoom < 1.0f) Zoom = 1.0f;
        if(Zoom > 45.0f) Zoom = 45.0f;
    }
    void setLastMouse(bool set){m_firstMouse = set;}

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