#pragma once

#include "Component.hpp"
#include "Property.hpp"

#include <typeinfo>

#include <glm/glm.hpp>

#include "../../Resource/Types/Mesh.hpp"
#include "../../Resource/Types/Texture.hpp"
#include "../../Renderer/Material.hpp"

namespace GNC {

// Mesh component
// Manages the visual representation of an entity by handling its 3D mesh and material.
// It only describes what to draw: GeometryPass records the actual draw for
// every visible entity that has one. A null material draws with Material's
// defaults.
class MeshComponent : public Component {
private:
    Mesh* mesh = nullptr;
    Material* material = nullptr;

public:
    GNC_COMPONENT_TYPE(MeshComponent, "Mesh")

    MeshComponent(Mesh* m, Material* mat) : mesh(m), material(mat) {}

    void SetMesh(Mesh* m) { mesh = m; }
    void SetMaterial(Material* mat) { material = mat; }

    Mesh* GetMesh() const { return mesh; }
    Material* GetMaterial() const { return material; }

    // The material's factors are listed as the mesh's own, so editing a
    // material that several meshes share changes all of them.
    static void Reflect(PropertyList<MeshComponent>& properties) {
        properties.Add<Resource*>(
            { .name = "Mesh", .resourceType = &typeid(Mesh) },
            [](const MeshComponent& component) -> Resource* { return component.GetMesh(); },
            [](MeshComponent& component, Resource* value) { component.SetMesh(dynamic_cast<Mesh*>(value)); });

        auto hasMaterial = [](const MeshComponent& component, PropertyInfo& info) { info.visible = component.GetMaterial() != nullptr; };

        properties.Add<glm::vec3>(
            { .name = "Base Color", .category = "Material", .hint = PropertyHint::Color },
            [](const MeshComponent& component) { return glm::vec3(MaterialOf(component).baseColorFactor); },
            [](MeshComponent& component, const glm::vec3& value) {
                if (Material* material = component.GetMaterial()) material->baseColorFactor = glm::vec4(value, material->baseColorFactor.a);
            },
            hasMaterial);

        properties.Add<float>(
            { .name = "Metallic", .category = "Material", .min = 0.0f, .max = 1.0f, .step = 0.005f },
            [](const MeshComponent& component) { return MaterialOf(component).metallicFactor; },
            [](MeshComponent& component, float value) {
                if (Material* material = component.GetMaterial()) material->metallicFactor = value;
            },
            hasMaterial);

        properties.Add<float>(
            { .name = "Roughness", .category = "Material", .min = 0.0f, .max = 1.0f, .step = 0.005f },
            [](const MeshComponent& component) { return MaterialOf(component).roughnessFactor; },
            [](MeshComponent& component, float value) {
                if (Material* material = component.GetMaterial()) material->roughnessFactor = value;
            },
            hasMaterial);

        properties.Add<glm::vec3>(
            { .name = "Emissive", .category = "Material", .hint = PropertyHint::Color },
            [](const MeshComponent& component) { return MaterialOf(component).emissiveFactor; },
            [](MeshComponent& component, const glm::vec3& value) {
                if (Material* material = component.GetMaterial()) material->emissiveFactor = value;
            },
            hasMaterial);

        properties.Add<float>(
            { .name = "Strength", .category = "Material", .min = 0.0f, .step = 0.05f, .format = "%.2f" },
            [](const MeshComponent& component) { return MaterialOf(component).emissiveStrength; },
            [](MeshComponent& component, float value) {
                if (Material* material = component.GetMaterial()) material->emissiveStrength = value;
            },
            hasMaterial);

        auto addMap = [&](const char* name, Texture* Material::* map) {
            properties.Add<Resource*>(
                { .name = name, .category = "Material", .resourceType = &typeid(Texture) },
                [map](const MeshComponent& component) -> Resource* { return MaterialOf(component).*map; },
                [map](MeshComponent& component, Resource* value) {
                    if (Material* material = component.GetMaterial()) material->*map = dynamic_cast<Texture*>(value);
                },
                hasMaterial);
        };
        addMap("Base Color Map", &Material::baseColorTexture);
        addMap("Metallic Roughness Map", &Material::metallicRoughnessTexture);
        addMap("Normal Map", &Material::normalTexture);
        addMap("Occlusion Map", &Material::occlusionTexture);
        addMap("Emissive Map", &Material::emissiveTexture);

        properties.Add<int>(
            { .name = "Alpha Mode", .category = "Material", .hint = PropertyHint::Enum, .enumNames = { "Opaque", "Mask", "Blend (drawn opaque)" } },
            [](const MeshComponent& component) { return static_cast<int>(MaterialOf(component).alphaMode); },
            [](MeshComponent& component, int value) {
                if (Material* material = component.GetMaterial()) material->alphaMode = static_cast<AlphaMode>(value);
            },
            hasMaterial);

        properties.Add<float>(
            { .name = "Alpha Cutoff", .category = "Material", .min = 0.0f, .max = 1.0f, .step = 0.005f },
            [](const MeshComponent& component) { return MaterialOf(component).alphaCutoff; },
            [](MeshComponent& component, float value) {
                if (Material* material = component.GetMaterial()) material->alphaCutoff = value;
            },
            [](const MeshComponent& component, PropertyInfo& info) {
                info.visible = component.GetMaterial() && component.GetMaterial()->alphaMode == AlphaMode::Mask;
            });

        properties.Add<bool>(
            { .name = "Double Sided", .category = "Material" },
            [](const MeshComponent& component) { return MaterialOf(component).doubleSided; },
            [](MeshComponent& component, bool value) {
                if (Material* material = component.GetMaterial()) material->doubleSided = value;
            },
            hasMaterial);
    }

private:
    // Keeps the material getters safe on a mesh without a material.
    static const Material& MaterialOf(const MeshComponent& component) {
        static const Material defaults {};
        return component.material ? *component.material : defaults;
    }
};

} // namespace GNC
