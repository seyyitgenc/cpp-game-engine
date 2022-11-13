#pragma once
#include <vector>
#include <memory>

#include "../ECS.h"
#include "../Components/Component.h"
#include "../Components/Transform.h"
class Entity
{
public:
    Entity()
    {
        this->addComponent<Transform>(0, 0); // sets newly created entity location
    };
    virtual ~Entity() = default;
    // THIS SECTION IS QUIET IMPORTANT
    // TODO: TRY TO UNDERSTAND THIS SECTION
    // TODO: UNDERSTAND UNIQUE AND SMART POINTERS
    template <typename T, typename... TArgs>
    T &addComponent(TArgs &&...args)
    {
        // FIXME if entered parameters doesn't correspond any constrcutor relatod to Component this will give error
        T *comp(new T(std::forward<TArgs>(args)...));
        std::unique_ptr<Component> uptr(comp);
        components.emplace_back(std::move(uptr));

        if (comp->init())
        {
            compList[getComponentTypeID<T>()] = comp;
            compBitset[getComponentTypeID<T>()] = true;
            comp->entity = this; // attach component to it's entity
            return *comp;        // return comp referance
        }
        return *static_cast<T *>(nullptr); // return null referance
    }
    template <typename T>
    T &getComponent() const
    {
        auto ptr(compList[getComponentTypeID<T>()]);
        return *static_cast<T *>(ptr);
    }

    template <typename T>
    // this const expression makes code safer
    bool hasComponent() const
    {
        return compBitset[getComponentTypeID<T>()];
    }

    bool isActive() const
    {
        return active;
    }
    void destroy()
    {
        active = false;
    }
    void draw()
    {
        for (auto &comp : components)
        {
            comp->draw();
        }
    }
    void update()
    {
        for (auto &comp : components)
        {
            comp->update();
        }
    }

private:
    bool active;
    ComponentList compList;
    ComponentBitset compBitset;
    std::vector<std::unique_ptr<Component>> components;
};