#pragma once

#include <memory>
#include <vector>

#include "../shader.h"

namespace GNC {

class Entity;

class Scene {
private:
public:
    void event(float deltaTime);
    void update(float deltaTime);

    void draw(const Shader& shader);

    void addEntity(std::unique_ptr<Entity> entity);

private:
    // TODO: wrap this container into thread-safe structure
    // NOTE: i am using ptrs because of preventing slicing issue
    std::vector<std::unique_ptr<Entity>> m_entityList;
};

} // namespace GNC
