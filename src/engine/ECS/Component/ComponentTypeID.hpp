#pragma once

#include <cstdint>

using ComponentTypeID = std::uint32_t;

// Component type identification
// Hands every component type a unique id the first time it is asked for, so an
// entity can look a component up by id instead of dynamic_cast-ing its way
// through the whole component list.
class ComponentTypeIDSystem {
private:
    static inline ComponentTypeID nextTypeID = 0;

public:
    template<typename T>
    static ComponentTypeID GetTypeID() {
        // One counter slot per T, assigned on first use and never again.
        static const ComponentTypeID typeID = nextTypeID++;
        return typeID;
    }
};
