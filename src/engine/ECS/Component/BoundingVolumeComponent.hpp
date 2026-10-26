#pragma once

#include "Component.hpp"
#include "MeshComponent.hpp"
#include "Property.hpp"

#include "../Entity/Entity.hpp"

#include "../../Culling/BoundingVolume.hpp"

#include <optional>
#include <vector>

#include <glm/glm.hpp>

namespace GNC {

// Bounding volume component
// The entity's extent in model space, fitted to the vertices of its mesh,
// which FrustumCulling tests against the camera. A sphere tests fastest and
// fits loosest, a convex hull fits tightest and tests slowest. An entity
// with a mesh but no bounding volume is never culled.
class BoundingVolumeComponent : public Component {
public:
    enum class Shape {
        Box,
        Sphere,
        ConvexHull
    };

private:
    Shape shape;
    BoundingVolume volume;

    // What volume was fitted to. The mesh is compared, never dereferenced:
    // meshes stay loaded, so a new one cannot take a fitted one's address.
    const Mesh* fittedMesh = nullptr;
    std::optional<Shape> fittedShape;

public:
    GNC_COMPONENT_TYPE(BoundingVolumeComponent, "Bounding Volume")

    explicit BoundingVolumeComponent(Shape volumeShape = Shape::Box) : shape(volumeShape) {}

    void SetShape(Shape volumeShape) { shape = volumeShape; }
    Shape GetShape() const { return shape; }

    // Refits first if the entity's mesh or the shape changed since the last
    // fit, which for a big mesh's hull takes a while. nullptr without a mesh.
    const BoundingVolume* GetVolume() {
        const MeshComponent* meshComponent = owner ? owner->GetComponent<MeshComponent>() : nullptr;
        const Mesh* mesh = meshComponent ? meshComponent->GetMesh() : nullptr;
        if (!mesh || mesh->GetVertices().empty()) return nullptr;

        if (mesh != fittedMesh || shape != fittedShape) Fit(*mesh);
        return &volume;
    }

    static void Reflect(PropertyList<BoundingVolumeComponent>& properties) {
        properties.Add<int>(
            { .name = "Shape", .hint = PropertyHint::Enum, .enumNames = { "Box", "Sphere", "Convex Hull" } }, // Shape's order
            [](const BoundingVolumeComponent& bounds) { return static_cast<int>(bounds.GetShape()); },
            [](BoundingVolumeComponent& bounds, int value) { bounds.SetShape(static_cast<Shape>(value)); });
    }

private:
    void Fit(const Mesh& mesh) {
        std::vector<glm::vec3> positions;
        positions.reserve(mesh.GetVertices().size());
        for (const Vertex& vertex : mesh.GetVertices()) {
            positions.push_back(vertex.position);
        }

        switch (shape) {
            case Shape::Box: volume = AABBBoundingBox3D::FromPoints(positions); break;
            case Shape::Sphere: volume = BoundingSphere::FromPoints(positions); break;
            case Shape::ConvexHull: volume = ConvexHull::FromPoints(positions); break;
        }
        fittedMesh = &mesh;
        fittedShape = shape;
    }
};

} // namespace GNC
