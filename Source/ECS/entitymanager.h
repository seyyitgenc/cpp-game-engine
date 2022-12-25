#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "entity.h"

class Manager
{
public:
    Manager() = default;
    void draw();
    void update(float &dt);
    void refresh();

    Entity &addEntity();
    void eraseEntity(Entity *entity);

private:
    std::vector<std::unique_ptr<Entity>> entities;
};