#pragma once
#include "globals.h"

#include <string>
#include <iostream>

struct LightProperties{
    glm::vec3 color = {1.0f, 1.0f, 1.0f}; // derive this color into ambient, diffuse, specular
    float constant  = 1.0f;
    float linear    = 0.09f;
    float quadratic = 0.032f;
    float shininess = 64;
    virtual ~LightProperties() = default;
};

struct DirectionalLightProperties : public LightProperties{
    glm::vec3 direction = {0.0f, -1.0f, 0.0f};
};


struct PointLightProperties : public LightProperties{
    // todo: add radius
    glm::vec3 position = {0.0f, 2.0f, 0.0f};
};

struct SpotLightProperties : public LightProperties{
    glm::vec3 position  = {0.0f, 5.0f, 0.0f};
    glm::vec3 direction = {0.2f, -1.0f,0.0f};
    float cutOff        = glm::cos(glm::radians(12.5f));
    float outerCutOff   = glm::cos(glm::radians(17.5f));
};
enum struct LightSpec{
    Point,
    Directional,
    Spot,
};


// TODO: Pointers to pass float variables here. ! MAYBE !
// NOTE:
// If you add new LightSpec you need to implement it first and then you need to add
// to the isValid function.
// // WORST ENGLISH EVER.

template <LightSpec Spec>
class Light
{
public:
    Light(std::unique_ptr<LightProperties> lightProp) : lightProp(std::move(lightProp)) { isValid(); };
    // todo: add different constructors for each light type that takes float and glm::vec3 parameters
    void setUniforms(const std::string &name){};
    // todo: i can add getLightProp here
    ~Light() = default;

    // NOTE: THE TYPE YOU ARE CASTED IS IMPORTANT. MAKE SURE YOU ARE AWARE WHAT YOU ARE DOING!
    template<typename T>
    T* getProperties(){
        return dynamic_cast<T*>(lightProp.get());
    }
private:
    std::unique_ptr<LightProperties> lightProp;
    void isValid(){
        static_assert(
            Spec == LightSpec::Directional || 
            Spec == LightSpec::Point || 
            Spec == LightSpec::Spot,
            "there is no implementation for this LightSpec");
    }
};
template<> inline void Light <LightSpec::Directional>::setUniforms(const std::string &name){
    if (gShaderManager->bind(name)){
        Shader* shader = gShaderManager->getShader(name);
        auto prop = static_cast<DirectionalLightProperties*>(lightProp.get());
        shader->setVec3("light.direction",  prop->direction);
        shader->setVec3("light.color",      prop->color);
        shader->setFloat("light.constant",  prop->constant);
        shader->setFloat("light.linear",    prop->linear);
        shader->setFloat("light.quadratic", prop->quadratic);
        shader->setFloat("shininess",       prop->shininess);
    }
}

template<> inline void Light <LightSpec::Point>::setUniforms(const std::string &name){
    if (gShaderManager->bind(name)){
        Shader* shader = gShaderManager->getShader(name);
        auto prop = static_cast<PointLightProperties*>(lightProp.get());
        shader->setVec3("light.position",   prop->position);
        shader->setVec3("light.color",      prop->color);
        shader->setFloat("light.constant",  prop->constant);
        shader->setFloat("light.linear",    prop->linear);
        shader->setFloat("light.quadratic", prop->quadratic);
        shader->setFloat("shininess",       prop->shininess);
    }
}
// TODO: take shader pointer as variable 
template<> inline void Light <LightSpec::Spot>::setUniforms(const std::string &name){
    if(gShaderManager->bind(name)){
        Shader* shader = gShaderManager->getShader(name);
        auto prop = static_cast<SpotLightProperties*>(lightProp.get());
        shader->setVec3("light.position",       prop->position);
        shader->setVec3("light.direction",      prop->direction);
        shader->setVec3("light.color",          prop->color);
        shader->setFloat("light.cutOff",        prop->cutOff);
        shader->setFloat("light.outerCutOff",   prop->outerCutOff);
        shader->setFloat("light.constant",      prop->constant);
        shader->setFloat("light.linear",        prop->linear);
        shader->setFloat("light.quadratic",     prop->quadratic);
        shader->setFloat("shininess",           prop->shininess);
    }
}

inline void testLights(){
    auto testingProp1 = std::make_unique<DirectionalLightProperties>();
    testingProp1->color = glm::vec3{1,1,1};
    testingProp1->direction = glm::vec3{1,0,2};
    testingProp1->constant = 75.0f;
    testingProp1->linear = 43.0f;
    testingProp1->quadratic = 113.0f;

    // auto dLight = Light<LightSpec::Directional>(std::move(testingProp1));
    // dLight.setUniforms("shader_model");
    // auto sLight = Light<LightSpec::Spot>(std::make_unique<SpotLightProperties>());
    // sLight.setUniforms("shader_model");
    auto pLight = Light<LightSpec::Point>(std::make_unique<PointLightProperties>());
    pLight.setUniforms("shader_model");
    // auto nLight = Light<LightSpec::None>(color,1,1,1);
    // nLight.setUniforms("test");
}