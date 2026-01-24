#include "scene.hpp"

#include "../shader.h"

#include "../entity/entity.hpp"

#include "../util/log.hpp"

namespace GNC {

void Scene::event(float deltaTime)
{
    // Handle events for each entity
    for (auto& entity : m_entityList) {
        entity->event(deltaTime); // Assuming Entity has an event method
    }
}

void Scene::update(float deltaTime)
{
    // Handle updates for each entity
    for (auto& entity : m_entityList) {
        entity->update(deltaTime); // Assuming Entity has an update method
    }
}

void Scene::draw(const Shader& shader)
{
    // Draw each entity
    // TODO: get drawable component of this entity and draw that
    for (auto& entity : m_entityList) {
        entity->draw(shader); // Assuming Entity has a draw method
    }
}

void Scene::addEntity(std::unique_ptr<Entity> entity)
{
    m_entityList.push_back(std::move(entity));
}

} // namespace GNC
