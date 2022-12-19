#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "entity.h"

class Manager
{
public:
    void draw();
    void update(float &dt);
    void refresh();

    static Manager &get()
    {
        if (s_instance == nullptr)
        {
            s_instance = new Manager();
        }
        return *s_instance;
    }

    Entity &addEntity();
    void eraseEntity(Entity *entity);

private:
    Manager() = default;
    static Manager *s_instance;
    std::vector<std::unique_ptr<Entity>> entities;
};