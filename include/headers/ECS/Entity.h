#pragma once
#include <vector>
#include <memory>

#include "ECS.h"
#include "Component.h"
#include "Transform.h"
class Entity
{
public:
    Entity()
    {
        this->addComponent<Transform>(0, 0); // sets newly created entity location
    };
    virtual ~Entity() = default;

    template <typename T, typename... TArgs>
    inline T &addComponent(TArgs &&...args)
    {
        // TODO: TRY TO UNDERSTAND THIS SECTION
        T *comp(new T(std::forward<TArgs>(args)...));
        // TODO: UNDERSTAND UNIQUE AND SMART POINTERS
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
    inline T &getComponent() const
    {
        auto ptr(compList[getComponentTypeID<T>()]);
        return *static_cast<T *>(ptr);
    }

    template <typename T>
    // this const expression makes code safer
    inline bool hasComponent() const
    {
        return compBitset[getComponentTypeID<T>()];
    }

    inline bool isActive() const
    {
        return active;
    }
    inline void destro()
    {
        active = false;
    }
    inline void draw()
    {
        for (auto &comp : components)
        {
            comp->draw();
        }
    }
    inline void update()
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