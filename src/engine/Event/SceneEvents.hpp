#pragma once

#include "Event.hpp"

#include <filesystem>
#include <string>
#include <utility>

#include "ECS/Component/ComponentTypeID.hpp"
#include "Scene/SceneSettings.hpp"

namespace GNC {

class Entity;

// Structural scene changes the UI requests instead of making them: it is in
// the middle of walking the entities when a menu item is picked. App applies
// them at the next Flush().

// A model file, imported with Scene/ModelImporter.
class ImportModelEvent : public Event {
private:
    std::filesystem::path path;

public:
    GNC_EVENT_TYPE(ImportModelEvent, EventCategory::Scene)

    explicit ImportModelEvent(std::filesystem::path file) : path(std::move(file)) {}

    const std::filesystem::path& GetPath() const { return path; }
};

// ------------
// Whole Scenes
// ------------
// Each replaces the scene; what the old one had is gone.
class NewSceneEvent : public Event {
private:
    SceneDimension dimension;

public:
    GNC_EVENT_TYPE(NewSceneEvent, EventCategory::Scene)

    explicit NewSceneEvent(SceneDimension sceneDimension) : dimension(sceneDimension) {}

    SceneDimension GetDimension() const { return dimension; }
};

// A .gncscene file (Scene/SceneSerializer).
class OpenSceneEvent : public Event {
private:
    std::filesystem::path path;

public:
    GNC_EVENT_TYPE(OpenSceneEvent, EventCategory::Scene)

    explicit OpenSceneEvent(std::filesystem::path file) : path(std::move(file)) {}

    const std::filesystem::path& GetPath() const { return path; }
};

// Saves the scene to path, which becomes the scene's file.
class SaveSceneEvent : public Event {
private:
    std::filesystem::path path;

public:
    GNC_EVENT_TYPE(SaveSceneEvent, EventCategory::Scene)

    explicit SaveSceneEvent(std::filesystem::path file) : path(std::move(file)) {}

    const std::filesystem::path& GetPath() const { return path; }
};

// These name an entity by address. A DestroyEntityEvent earlier in the same
// Flush() may have destroyed it: check Scene::Contains() before using it.

// At the root of the scene, or under parent.
class CreateEntityEvent : public Event {
private:
    std::string entityName;
    Entity* parent;

public:
    GNC_EVENT_TYPE(CreateEntityEvent, EventCategory::Scene)

    explicit CreateEntityEvent(std::string name, Entity* parentEntity = nullptr) : entityName(std::move(name)), parent(parentEntity) {}

    // Not GetName(), which every event has for its own type's name.
    const std::string& GetEntityName() const { return entityName; }
    Entity* GetParent() const { return parent; }
};

class AddComponentEvent : public Event {
private:
    Entity* entity;
    ComponentTypeID componentTypeID;

public:
    GNC_EVENT_TYPE(AddComponentEvent, EventCategory::Scene)

    AddComponentEvent(Entity& target, ComponentTypeID typeID) : entity(&target), componentTypeID(typeID) {}

    Entity* GetEntity() const { return entity; }
    ComponentTypeID GetComponentTypeID() const { return componentTypeID; }
};

class RemoveComponentEvent : public Event {
private:
    Entity* entity;
    ComponentTypeID componentTypeID;

public:
    GNC_EVENT_TYPE(RemoveComponentEvent, EventCategory::Scene)

    RemoveComponentEvent(Entity& target, ComponentTypeID typeID) : entity(&target), componentTypeID(typeID) {}

    Entity* GetEntity() const { return entity; }
    ComponentTypeID GetComponentTypeID() const { return componentTypeID; }
};

// Its descendants go with it.
class DestroyEntityEvent : public Event {
private:
    Entity* entity;

public:
    GNC_EVENT_TYPE(DestroyEntityEvent, EventCategory::Scene)

    explicit DestroyEntityEvent(Entity& target) : entity(&target) {}

    Entity* GetEntity() const { return entity; }
};

} // namespace GNC
