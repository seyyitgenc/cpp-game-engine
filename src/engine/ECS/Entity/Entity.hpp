#pragma once

#include "../Component/Component.hpp"
#include "../Component/ComponentTypeID.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace GNC {

// Entity class
// A name plus the components it owns, with a type id -> component map for
// O(1) lookups.

// FIXME: not thread safe
class Entity {
private:
    std::string name;
    bool active = true;
    bool initialized = false;

    std::vector<std::unique_ptr<Component>> components;
    std::unordered_map<ComponentTypeID, Component*> componentMap;

public:
    explicit Entity(const std::string& entityName) : name(entityName) {}

    ~Entity() { Destroy(); }

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    const std::string& GetName() const { return name; }
    void SetName(const std::string& entityName) { name = entityName; }
    bool IsActive() const { return active; }
    void SetActive(bool isActive) { active = isActive; }

    // In the order added.
    const std::vector<std::unique_ptr<Component>>& GetComponents() const { return components; }

    void Initialize() {
        for (auto& component : components) {
            component->Initialize();
        }
        initialized = true;
    }

    void Update(float deltaTime) {
        if (!active) return;

        for (auto& component : components) {
            if (!component->IsActive()) continue;
            component->Update(deltaTime);
        }
    }

    void Render() {
        if (!active) return;

        for (auto& component : components) {
            if (!component->IsActive()) continue;
            component->Render();
        }
    }

    void Destroy() {
        // Tear down in reverse order of construction, so a component can still
        // reach the ones it was built on top of.
        for (auto it = components.rbegin(); it != components.rend(); ++it) {
            (*it)->Destroy();
        }
        componentMap.clear();
        components.clear();
        initialized = false;
    }

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        const ComponentTypeID typeID = ComponentTypeIDSystem::GetTypeID<T>();

        // One component per type: hand back what is already there.
        if (auto it = componentMap.find(typeID); it != componentMap.end()) {
            return static_cast<T*>(it->second);
        }

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();
        componentPtr->SetOwner(this);

        components.push_back(std::move(component));
        componentMap[typeID] = componentPtr;

        // A component added after Initialize() is initialized as it joins.
        if (initialized) {
            componentPtr->Initialize();
        }
        return componentPtr;
    }

    template<typename T>
    T* GetComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        const ComponentTypeID typeID = ComponentTypeIDSystem::GetTypeID<T>();
        if (auto it = componentMap.find(typeID); it != componentMap.end()) {
            // The map key is the exact type, so no dynamic_cast is needed.
            return static_cast<T*>(it->second);
        }
        return nullptr;
    }

    template<typename T>
    bool HasComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        return HasComponent(ComponentTypeIDSystem::GetTypeID<T>());
    }

    bool HasComponent(ComponentTypeID typeID) const { return componentMap.contains(typeID); }

    template<typename T>
    bool RemoveComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        return RemoveComponent(ComponentTypeIDSystem::GetTypeID<T>());
    }

    bool RemoveComponent(ComponentTypeID typeID) {
        auto mapped = componentMap.find(typeID);
        if (mapped == componentMap.end()) return false;

        Component* target = mapped->second;
        target->Destroy();
        componentMap.erase(mapped);

        for (auto it = components.begin(); it != components.end(); ++it) {
            if (it->get() == target) {
                components.erase(it);
                return true;
            }
        }
        return false;
    }
};

} // namespace GNC
