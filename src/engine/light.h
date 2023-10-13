#include <string>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct DirectionalLight
{
    glm::vec3 direction;
    glm::vec3 color;
    float constant; 
    float linear;
    float quadratic;
    DirectionalLight(glm::vec3 direction, glm::vec3 color, float constant, float linear, float quadratic)
     : direction(direction), color(color), constant(constant), linear(linear), quadratic(quadratic) {}
    ~DirectionalLight() = default;
};

struct PointLight
{
    glm::vec3 position;
    glm::vec3 color;
    float constant; 
    float linear;
    float quadratic;
    PointLight(glm::vec3 position, glm::vec3 color, float constant, float linear, float quadratic)
     : position(position), color(color), constant(constant), linear(linear), quadratic(quadratic) {}
     ~PointLight() = default;
};

struct SpotLight
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 direction;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    SpotLight(glm::vec3 position, glm::vec3 color, glm::vec3 direction, float constant, float linear, float quadratic, float cutOff, float outerCutOff)
     : position(position), color(color), direction(direction), constant(constant), linear(linear), quadratic(quadratic), cutOff(cutOff), outerCutOff(outerCutOff) {}
     ~SpotLight() = default;
};


