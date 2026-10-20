#pragma once

#include "Component.hpp"

#include "../../Resource/Types/Mesh.hpp"
#include "../../Renderer/Material.hpp"

#include "../../BoundingBox.hpp"

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
    MeshComponent(Mesh* m, Material* mat) : mesh(m), material(mat) {}

    void SetMesh(Mesh* m) { mesh = m; }
    void SetMaterial(Material* mat) { material = mat; }

    Mesh* GetMesh() const { return mesh; }
    Material* GetMaterial() const { return material; }

    // FIXME: later on calculate the bounding box properly, bounding box can be, spherical, square, etc...
    // min has to stay the small corner and max the big one, otherwise Center()
    // and Extents() come out inverted and the box never passes the frustum test.
    AABBBoundingBox3D GetBoundingBox() const { return {{-32, -32, -32}, {32, 32, 32}}; }
};

} // namespace GNC
