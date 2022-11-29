#pragma once

#include <vector>
#include <memory>
#include "Entity.h"

class Manager
{
public:
    Manager() = default;
    ~Manager() = default;

    void draw();
    void update(float &dt);
    void refresh();

    Entity& addEntity();
    void eraseEntity(Entity *entity);

private:
    std::vector<std::unique_ptr<Entity>> entities;
};