#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ECS/Entity/Entity.hpp"
#include "Renderer/Material.hpp"
#include "SceneSettings.hpp"

namespace GNC {

// Scene
// Owns the entities and the materials their MeshComponents point at, behind
// unique_ptr so addresses stay valid. Materials are declared first, so they
// outlive the entities. Entities form a tree through their parents; the
// list stays flat, in the order created. Scene files (SceneSerializer) save
// and load all of it, with the settings.
class Scene {
private:
    std::string name = "Untitled";
    SceneDimension dimension = SceneDimension::ThreeD;
    SceneEnvironment environment;
    // The one the game sees through; one of the entities, or null.
    Entity* camera = nullptr;

    std::vector<std::unique_ptr<Material>> materials;
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<Entity*> entityViews; // the form Renderer::Render() takes
    std::unordered_map<std::uint64_t, Entity*> entitiesById;

public:
    Scene() = default;
    Scene(Scene&&) = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Replaces this scene with other: the entities go before the materials
    // they point at. Whatever points into this scene must let go first.
    Scene& operator=(Scene&& other) noexcept {
        if (this == &other) return *this;
        camera = nullptr;
        entitiesById.clear();
        entityViews.clear();
        entities.clear();
        materials.clear();

        name = std::move(other.name);
        dimension = other.dimension;
        environment = other.environment;
        camera = std::exchange(other.camera, nullptr);
        materials = std::move(other.materials);
        entities = std::move(other.entities);
        entityViews = std::move(other.entityViews);
        entitiesById = std::move(other.entitiesById);
        return *this;
    }

    // --------
    // Settings
    // --------
    const std::string& GetName() const { return name; }
    void SetName(std::string sceneName) { name = std::move(sceneName); }

    SceneDimension GetDimension() const { return dimension; }
    void SetDimension(SceneDimension value) { dimension = value; }

    const SceneEnvironment& GetEnvironment() const { return environment; }
    void SetEnvironment(const SceneEnvironment& value) { environment = value; }

    Entity* GetCamera() const { return camera; }
    // An entity of this scene, or null.
    void SetCamera(Entity* entity) { camera = entity; }

    // --------
    // Entities
    // --------
    // Already initialized, so components added afterwards initialize as they
    // join. parent, if any, must be in this scene. id 0, or one another
    // entity has, gets a new random id.
    Entity& CreateEntity(const std::string& entityName, Entity* parent = nullptr, std::uint64_t id = 0) {
        entities.push_back(std::make_unique<Entity>(entityName));
        Entity& entity = *entities.back();
        while (id == 0 || entitiesById.contains(id)) id = RandomId();
        entity.id = id;
        entitiesById.emplace(id, &entity);
        if (parent) {
            entity.parent = parent;
            parent->children.push_back(&entity);
        }
        entity.Initialize();
        entityViews.push_back(&entity);
        return entity;
    }

    Entity* FindEntity(std::uint64_t id) const {
        const auto it = entitiesById.find(id);
        return it != entitiesById.end() ? it->second : nullptr;
    }

    // name, or "name 1", "name 2", ... when a sibling already has it.
    std::string MakeUniqueName(const std::string& entityName, const Entity* parent = nullptr) const {
        auto taken = [&](const std::string& candidate) {
            return std::ranges::any_of(entityViews, [&](const Entity* entity) {
                return entity->parent == parent && entity->GetName() == candidate;
            });
        };

        if (!taken(entityName)) return entityName;
        for (int number = 1;; ++number) {
            std::string candidate = entityName + " " + std::to_string(number);
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
        if (doomed.contains(camera)) camera = nullptr;
        for (const Entity* gone : subtree) entitiesById.erase(gone->id);
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

private:
    static std::uint64_t RandomId() {
        static std::mt19937_64 engine(std::random_device {}());
        return engine();
    }
};

} // namespace GNC
