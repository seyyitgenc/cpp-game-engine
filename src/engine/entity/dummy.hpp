#pragma once

#include "entity.hpp"

#include "../model.h"

namespace GNC {

class Dummy : public Entity {
public:
    Dummy(const std::string& modelPath);

    virtual void event(float deltaTime) override;
    virtual void update(float deltaTime) override;
    virtual void draw(const Shader& shader) override;

    virtual ~Dummy() { }

    // TODO: later on abstract these into transform component
    void setPosition(const glm::vec3& position) { m_position = position; }
    void setScale(const glm::vec3& scale) { m_scale = scale; }

private:
    std::unique_ptr<Model> m_model;

    // TODO: later on abstract these into transform component
    glm::vec3 m_position {};
    glm::vec3 m_scale {};
};

} // namespace GNC
