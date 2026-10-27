#pragma once

#include "Component.hpp"
#include "Property.hpp"

#include <cstddef>
#include <typeinfo>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "../../Resource/Types/Mesh.hpp"
#include "../../Resource/Types/Texture.hpp"
#include "../../Renderer/Material.hpp"

namespace GNC {

// Mesh component
// Manages the visual representation of an entity by handling its 3D mesh and
// a material per submesh (slot i for submesh i), as Unity's MeshRenderer
// does. It only describes what to draw: GeometryPass records the draws for
// every visible entity that has one. A missing or null material draws with
// Material's defaults.
class MeshComponent : public Component {
private:
    Mesh* mesh = nullptr;
    // Not resized with the mesh: switching to a mesh with fewer submeshes
    // and back keeps the slots.
    std::vector<Material*> materials;

public:
    GNC_COMPONENT_TYPE(MeshComponent, "Mesh")

    MeshComponent(Mesh* m, std::vector<Material*> mats) : mesh(m), materials(std::move(mats)) {}
    MeshComponent(Mesh* m, Material* mat) : MeshComponent(m, std::vector<Material*> { mat }) {}

    void SetMesh(Mesh* m) { mesh = m; }
    void SetMaterial(std::size_t slot, Material* mat) {
        if (slot >= materials.size()) materials.resize(slot + 1, nullptr);
        materials[slot] = mat;
    }

    Mesh* GetMesh() const { return mesh; }
    Material* GetMaterial(std::size_t slot) const { return slot < materials.size() ? materials[slot] : nullptr; }

    // The mesh's submeshes; without a mesh, the materials held.
    std::size_t GetSlotCount() const { return mesh ? mesh->GetSubMeshes().size() : materials.size(); }

    // Each slot's material factors are listed as the mesh's own, so editing a
    // material that several slots or meshes share changes all of them.
    static void Reflect(PropertyList<MeshComponent>& properties) {
        properties.Add<Resource*>(
            { .name = "Mesh", .resourceType = &typeid(Mesh) },
            [](const MeshComponent& component) -> Resource* { return component.GetMesh(); },
            [](MeshComponent& component, Resource* value) { component.SetMesh(dynamic_cast<Mesh*>(value)); });

        const auto slotCount = [](const MeshComponent& component) { return component.GetSlotCount(); };
        const auto hasMaterial = [](const MeshComponent& component, std::size_t slot, PropertyInfo& info) {
            info.visible = component.GetMaterial(slot) != nullptr;
        };

        properties.AddElement<glm::vec3>(
            { .name = "Base Color", .category = "Material", .hint = PropertyHint::Color }, slotCount,
            [](const MeshComponent& component, std::size_t slot) { return glm::vec3(MaterialOf(component, slot).baseColorFactor); },
            [](MeshComponent& component, std::size_t slot, const glm::vec3& value) {
                if (Material* material = component.GetMaterial(slot)) material->baseColorFactor = glm::vec4(value, material->baseColorFactor.a);
            },
            hasMaterial);

        // A material field shown as it is stored.
        auto addField = [&]<typename Value>(PropertyInfo info, Value Material::* field) {
            properties.AddElement<Value>(
                std::move(info), slotCount,
                [field](const MeshComponent& component, std::size_t slot) { return MaterialOf(component, slot).*field; },
                [field](MeshComponent& component, std::size_t slot, const Value& value) {
                    if (Material* material = component.GetMaterial(slot)) material->*field = value;
                },
                hasMaterial);
        };
        addField({ .name = "Metallic", .category = "Material", .min = 0.0f, .max = 1.0f, .step = 0.005f }, &Material::metallicFactor);
        addField({ .name = "Roughness", .category = "Material", .min = 0.0f, .max = 1.0f, .step = 0.005f }, &Material::roughnessFactor);
        addField({ .name = "Emissive", .category = "Material", .hint = PropertyHint::Color }, &Material::emissiveFactor);
        addField({ .name = "Strength", .category = "Material", .min = 0.0f, .step = 0.05f, .format = "%.2f" }, &Material::emissiveStrength);

        auto addMap = [&](const char* name, Texture* Material::* map) {
            properties.AddElement<Resource*>(
                { .name = name, .category = "Material", .resourceType = &typeid(Texture) }, slotCount,
                [map](const MeshComponent& component, std::size_t slot) -> Resource* { return MaterialOf(component, slot).*map; },
                [map](MeshComponent& component, std::size_t slot, Resource* value) {
                    if (Material* material = component.GetMaterial(slot)) material->*map = dynamic_cast<Texture*>(value);
                },
                hasMaterial);
        };
        addMap("Base Color Map", &Material::baseColorTexture);
        addMap("Metallic Roughness Map", &Material::metallicRoughnessTexture);
        addMap("Normal Map", &Material::normalTexture);
        addMap("Occlusion Map", &Material::occlusionTexture);
        addMap("Emissive Map", &Material::emissiveTexture);

        properties.AddElement<int>(
            { .name = "Alpha Mode", .category = "Material", .hint = PropertyHint::Enum, .enumNames = { "Opaque", "Mask", "Blend (drawn opaque)" } },
            slotCount,
            [](const MeshComponent& component, std::size_t slot) { return static_cast<int>(MaterialOf(component, slot).alphaMode); },
            [](MeshComponent& component, std::size_t slot, int value) {
                if (Material* material = component.GetMaterial(slot)) material->alphaMode = static_cast<AlphaMode>(value);
            },
            hasMaterial);

        properties.AddElement<float>(
            { .name = "Alpha Cutoff", .category = "Material", .min = 0.0f, .max = 1.0f, .step = 0.005f }, slotCount,
            [](const MeshComponent& component, std::size_t slot) { return MaterialOf(component, slot).alphaCutoff; },
            [](MeshComponent& component, std::size_t slot, float value) {
                if (Material* material = component.GetMaterial(slot)) material->alphaCutoff = value;
            },
            [](const MeshComponent& component, std::size_t slot, PropertyInfo& info) {
                const Material* material = component.GetMaterial(slot);
                info.visible = material && material->alphaMode == AlphaMode::Mask;
            });

        addField({ .name = "Double Sided", .category = "Material" }, &Material::doubleSided);
    }

private:
    // Keeps the material getters safe on a slot without a material.
    static const Material& MaterialOf(const MeshComponent& component, std::size_t slot) {
        static const Material defaults {};
        const Material* material = component.GetMaterial(slot);
        return material ? *material : defaults;
    }
};

} // namespace GNC
