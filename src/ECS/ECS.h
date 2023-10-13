#pragma once
#include <array>
#include <bitset>
#include <iostream>

class Entity;
class Component;

const std::size_t MAX_ENTITIES = 5000;
const std::size_t MAX_COMPONENTS = 32;

// ComponentTypeID alias replace this expression as std::size_t
// Increases code readability
using ComponentTypeID = std::size_t;

// define uid only once and increment it whenever function called
inline ComponentTypeID getUniqueComponentID() {
    static ComponentTypeID lastID = 0u;
    return lastID++;
}
template <typename T>
inline ComponentTypeID getComponentTypeID() noexcept {
    static_assert(std::is_base_of<Component, T>::value,
                  "Type not base on component!");
    static const ComponentTypeID typeID = getUniqueComponentID();
    return typeID;
}
using ComponentBitset = std::bitset<MAX_COMPONENTS>;
using ComponentList = std::array<Component *, MAX_COMPONENTS>;