#pragma once

#include "TransformsComponent.hpp"
#include "../Entity/Entity.hpp"


// TODO: implement these out
class Mesh {
public:
    void Render() { }
};

// TODO: implement these out
class Material {
public:

    void Bind() { }
    void SetUniform(const std::string&, glm::mat4 matrix) {}
};

// Mesh component
// Manages the visual representation of an entity by handling its 3D mesh and material
class MeshComponent : public Component {
private:
    Mesh* mesh = nullptr;
    Material* material = nullptr;

public:
    MeshComponent(Mesh* m, Material* mat) : mesh(m), material(mat) {}

    void SetMesh(Mesh* m) { mesh = m; }
    void SetMaterial(Material* mat) { material = mat; }

    Mesh* GetMesh() const { return mesh; }
    Material* GetMaterial() const { return material; }

    void Render() override {
        if (!mesh || !material) return;

        // Get transform component
        auto transform = GetOwner()->GetComponent<TransformComponent>();
        if (!transform) return;

        // Render mesh with material and transform
        material->Bind();
        material->SetUniform("modelMatrix", transform->GetTransformMatrix());
        mesh->Render();
    }
};
