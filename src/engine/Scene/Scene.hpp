#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "ECS/Entity/Entity.hpp"
#include "Renderer/Material.hpp"

namespace GNC {

// Scene
// Owns the entities, and the materials their MeshComponents point at. Both
// live behind unique_ptr, so their addresses stay valid as more are added.
// Materials are declared first so they outlive the entities using them.
class Scene {
private:
    std::vector<std::unique_ptr<Material>> materials;
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<Entity*> entityViews; // the form Renderer::Render() takes

public:
    // Comes back already initialized, so components added afterwards are
    // initialized as they join (see Entity::AddComponent) and are live at
    // once; there is no separate "start the scene" step to forget.
    Entity& CreateEntity(const std::string& name) {
        entities.push_back(std::make_unique<Entity>(name));
        Entity& entity = *entities.back();
        entity.Initialize();
        entityViews.push_back(&entity);
        return entity;
    }

    // name, or "name 1", "name 2", ... when an entity already has it.
    std::string MakeUniqueName(const std::string& name) const {
        auto taken = [this](const std::string& candidate) {
            return std::ranges::any_of(entities, [&](const std::unique_ptr<Entity>& entity) {
                return entity->GetName() == candidate;
            });
        };

        if (!taken(name)) return name;
        for (int number = 1;; ++number) {
            std::string candidate = name + " " + std::to_string(number);
            if (!taken(candidate)) return candidate;
        }
    }

    Material* CreateMaterial(const Material& material) {
        materials.push_back(std::make_unique<Material>(material));
        return materials.back().get();
    }

    void Update(float deltaTime) {
        for (const auto& entity : entities) {
            entity->Update(deltaTime);
        }
    }

    const std::vector<Entity*>& GetEntities() const { return entityViews; }
};

} // namespace GNC
