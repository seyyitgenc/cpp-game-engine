#pragma once

#include "Event.hpp"

#include <string>
#include <utility>

#include "ECS/Component/ComponentTypeID.hpp"

namespace GNC {

class Entity;

// Structural scene changes the UI requests instead of making them: it is in
// the middle of walking the entities when a menu item is picked. App applies
// them at the next Flush().

class CreateEntityEvent : public Event {
private:
    std::string entityName;

public:
    GNC_EVENT_TYPE(CreateEntityEvent, EventCategory::Scene)

    explicit CreateEntityEvent(std::string name) : entityName(std::move(name)) {}

    // Not GetName(), which every event has for its own type's name.
    const std::string& GetEntityName() const { return entityName; }
};

// These two point at the entity, which must still exist at Flush().
class AddComponentEvent : public Event {
private:
    Entity* entity;
    ComponentTypeID componentTypeID;

public:
    GNC_EVENT_TYPE(AddComponentEvent, EventCategory::Scene)

    AddComponentEvent(Entity& target, ComponentTypeID typeID) : entity(&target), componentTypeID(typeID) {}

    Entity& GetEntity() const { return *entity; }
    ComponentTypeID GetComponentTypeID() const { return componentTypeID; }
};

class RemoveComponentEvent : public Event {
private:
    Entity* entity;
    ComponentTypeID componentTypeID;

public:
    GNC_EVENT_TYPE(RemoveComponentEvent, EventCategory::Scene)

    RemoveComponentEvent(Entity& target, ComponentTypeID typeID) : entity(&target), componentTypeID(typeID) {}

    Entity& GetEntity() const { return *entity; }
    ComponentTypeID GetComponentTypeID() const { return componentTypeID; }
};

} // namespace GNC
