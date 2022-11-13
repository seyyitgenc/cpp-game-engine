#pragma once
#include <vector>
#include <memory>
#include <cassert>

#include "../ECS.h"
#include "../Component.h"
#include "../Components/Transform.h"
class Entity
{
public:
    Entity()
    {
        addComponent<Transform>(0, 0); // sets newly created entity location
    };
    virtual ~Entity() = default;
    // THIS SECTION IS QUIET IMPORTANT
    // TODO: TRY TO UNDERSTAND THIS SECTION
    // TODO: UNDERSTAND UNIQUE AND SMART POINTERS
    template <typename T, typename... TArgs>
    T &addComponent(TArgs &&...args)
    {
        // assert will check if this component exist. if exist it stops process.
        assert(!hasComponent<T>());
        // FIXME if entered parameters doesn't correspond any constrcutor relatod to Component this will give error
        T *comp(new T(std::forward<TArgs>(args)...));
        comp->entity = this; // attach component to it's entity
        std::unique_ptr<Component> uptr(comp);
        components.emplace_back(std::move(uptr));

        compList[getComponentTypeID<T>()] = comp;
        compBitset[getComponentTypeID<T>()] = true;

        // if component doesn't have initialize function it will return false and because of that
        // if you use it in if claus probably your function will fail
        comp->init();
        return *comp; // return comp referance
    }
    // Get specified component
    template <typename T>
    T &getComponent() const
    {
        // assert will check if this component exist then continue to process
        assert(hasComponent<T>());
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
    bool active{true};
    ComponentList compList;
    ComponentBitset compBitset;
    std::vector<std::unique_ptr<Component>> components;
};