#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "ECS/Entity/Entity.hpp"
#include "Renderer/Material.hpp"

namespace GNC {

// Scene
// Owns the entities and the materials their MeshComponents point at, behind
// unique_ptr so addresses stay valid. Materials are declared first, so they
// outlive the entities. Entities form a tree through their parents; the
// list stays flat, in the order created.
class Scene {
private:
    std::vector<std::unique_ptr<Material>> materials;
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<Entity*> entityViews; // the form Renderer::Render() takes

public:
    // Already initialized, so components added afterwards initialize as they
    // join. parent, if any, must be in this scene.
    Entity& CreateEntity(const std::string& name, Entity* parent = nullptr) {
        entities.push_back(std::make_unique<Entity>(name));
        Entity& entity = *entities.back();
        if (parent) {
            entity.parent = parent;
            parent->children.push_back(&entity);
        }
        entity.Initialize();
        entityViews.push_back(&entity);
        return entity;
    }

    // name, or "name 1", "name 2", ... when a sibling already has it.
    std::string MakeUniqueName(const std::string& name, const Entity* parent = nullptr) const {
        auto taken = [&](const std::string& candidate) {
            return std::ranges::any_of(entityViews, [&](const Entity* entity) {
                return entity->parent == parent && entity->GetName() == candidate;
            });
        };

        if (!taken(name)) return name;
        for (int number = 1;; ++number) {
            std::string candidate = name + " " + std::to_string(number);
            if (!taken(candidate)) return candidate;
        }
    }

    // Compares addresses only, so it is safe on one already destroyed.
    bool Contains(const Entity* entity) const {
        return std::ranges::find(entityViews, entity) != entityViews.end();
    }

    // root first, then its descendants.
    static std::vector<Entity*> GetSubtree(Entity& root) {
        std::vector<Entity*> subtree { &root };
        for (std::size_t i = 0; i < subtree.size(); ++i) {
            const std::vector<Entity*>& children = subtree[i]->children;
            subtree.insert(subtree.end(), children.begin(), children.end());
        }
        return subtree;
    }

    // Its descendants go with it, and their components with each; their
    // materials stay. Whatever else points at any of them (Selection,
    // Renderer::ForgetEntity()) must let go first.
    void DestroyEntity(Entity* entity) {
        if (entity->parent) std::erase(entity->parent->children, entity);

        const std::vector<Entity*> subtree = GetSubtree(*entity);
        const std::unordered_set<const Entity*> doomed(subtree.begin(), subtree.end());
        std::erase_if(entityViews, [&](const Entity* view) { return doomed.contains(view); });
        std::erase_if(entities, [&](const std::unique_ptr<Entity>& owned) { return doomed.contains(owned.get()); });
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
