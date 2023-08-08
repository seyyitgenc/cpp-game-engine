#pragma once
#include <algorithm>
#include <memory>
#include <vector>

#include "entity.h"

class Manager {
public:
    Manager() = default;
    void draw();
    void update(const float &dt);
    void refresh();

    Entity &addEntity();
    void eraseEntity(Entity *entity);

private:
    std::vector<std::unique_ptr<Entity>> entities;
};