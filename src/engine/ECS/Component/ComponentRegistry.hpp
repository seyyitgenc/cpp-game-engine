#pragma once

#include "Component.hpp"
#include "ComponentTypeID.hpp"
#include "Property.hpp"

#include "../Entity/Entity.hpp"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace GNC {

// Component registry
// The component types the editor can add and edit: each one's icon, how to
// add one with defaults, and its properties (from T::Reflect()). Components
// of an unregistered type still show in the UI, but cannot be added or
// edited there.
class ComponentRegistry {
public:
    struct Registration {
        std::string icon {};
        // Must add a T through Entity::AddComponent. Empty: a
        // default-constructed T.
        std::function<void(Entity&)> addTo {};
    };

    struct Entry {
        ComponentTypeID typeID;
        const char* name;
        std::string icon;
        std::function<void(Entity&)> addTo;
        std::vector<Property> properties;
    };

private:
    // In registration order, which the menus follow.
    std::vector<Entry> entries;

public:
    // Registering a type again does nothing. Throws std::invalid_argument
    // for a type without a default constructor and no addTo.
    template<typename T>
    void Register(Registration type) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

        if (Find(T::StaticTypeID())) return;

        if (!type.addTo) {
            if constexpr (std::is_default_constructible_v<T>) {
                type.addTo = [](Entity& entity) { entity.AddComponent<T>(); };
            } else {
                throw std::invalid_argument(std::string("ComponentRegistry: ") + T::StaticTypeName() + " needs an addTo");
            }
        }

        std::vector<Property> properties;
        if constexpr (requires(PropertyList<T>& list) { T::Reflect(list); }) {
            PropertyList<T> list;
            T::Reflect(list);
            properties = std::move(list).Release();
        }

        entries.push_back(Entry {
            .typeID = T::StaticTypeID(),
            .name = T::StaticTypeName(),
            .icon = std::move(type.icon),
            .addTo = std::move(type.addTo),
            .properties = std::move(properties)
        });
    }

    const std::vector<Entry>& GetEntries() const { return entries; }

    const Entry* Find(ComponentTypeID typeID) const {
        const auto it = std::ranges::find(entries, typeID, &Entry::typeID);
        return it != entries.end() ? &*it : nullptr;
    }
};

} // namespace GNC
