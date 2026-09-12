#pragma once

#include "../Entity/Entity.hpp"

#include "TransformsComponent.hpp"

#include "../../Resource/Types/Mesh.hpp"

#include "../../BoundingBox.hpp"

namespace GNC {

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

    // FIXME: later on calculate the bounding box properly, bounding box can be, spherical, square, etc...
    // min has to stay the small corner and max the big one, otherwise Center()
    // and Extents() come out inverted and the box never passes the frustum test.
    AABBBoundingBox3D GetBoundingBox() const { return {{-32, -32, -32}, {32, 32, 32}}; }

    void Render() override {
        if (!mesh || !material) return;

        // Get transform component
        auto transform = GetOwner()->GetComponent<TransformComponent>();
        if (!transform) return;

        // Render mesh with material and transform
        material->Bind();
        material->SetUniform("modelMatrix", transform->GetTransformMatrix());

        // TODO: the mesh cannot draw itself under Vulkan. This wants a command
        // buffer to record into: bind the pipeline, bind mesh->GetVertexBuffer()
        // and mesh->GetIndexBuffer(), then drawIndexed(mesh->GetIndexCount()).
    }
};

} // namespace GNC
