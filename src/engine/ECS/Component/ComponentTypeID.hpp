#pragma once

#include <cstdint>

namespace GNC {

using ComponentTypeID = std::uint32_t;

// A unique id per component type, assigned on first use, so entities look
// components up by id instead of dynamic_cast.
class ComponentTypeIDSystem {
private:
    static inline ComponentTypeID nextTypeID = 0;

public:
    template<typename T>
    static ComponentTypeID GetTypeID() {
        static const ComponentTypeID typeID = nextTypeID++;
        return typeID;
    }
};

} // namespace GNC
