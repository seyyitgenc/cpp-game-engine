#pragma once

#include <vector>
#include <memory>
#include "Entity.h"

class EntityManager
{
public:
    EntityManager() = default;
    ~EntityManager() = default;

    void draw();
    void update();
    void refresh();

    void addEntity(Entity *entity);
    void eraseEntity(Entity *entity);
    Entity *cloneEntity(Entity *entity);

private:
    std::vector<std::unique_ptr<Entity>> entities;
};