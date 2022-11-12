#include "EntityManager.h"

void EntityManager::draw()
{
    for (auto &entity : entities)
        entity->draw();
}
void EntityManager::update()
{
    for (auto &entity : entities)
        entity->update();
}
void EntityManager::refresh()
{
}

void EntityManager::addEntity(Entity *entity)
{
    std::unique_ptr<Entity> uniquePtr{entity};
    entities.emplace_back(std::move(uniquePtr));
}
void EntityManager::eraseEntity(Entity *entity)
{
}
Entity *EntityManager::cloneEntity(Entity *entity)
{
    return nullptr;
}