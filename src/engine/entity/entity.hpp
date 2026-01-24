#pragma once

#include "../shader.h"

#include <iostream>
// Default constructable entity placeholder
namespace GNC {

// abstract base class for entities
class Entity {
public:
    virtual void event([[maybe_unused]] float deltaTime) { /* default empty implementation */ };
    virtual void update([[maybe_unused]] float deltaTime) { /* default empty implementation */ };

    // TODO: later on abstract this into drawable component
    virtual void draw([[maybe_unused]] const Shader& shader)
    {
        std::cout << "Entity::draw called on base Entity class. This should be overridden in derived classes." << std::endl;
        /* default empty implementation */
    };

    virtual ~Entity() = default;
};

} // namespace GNC
