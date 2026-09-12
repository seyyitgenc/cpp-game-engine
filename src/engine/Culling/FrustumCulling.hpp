#pragma once

#include "../ECS/Entity/Entity.hpp"

#include "../ECS/Component/MeshComponent.hpp"
#include "../ECS/Component/TransformsComponent.hpp"

#include "../BoundingBox.hpp"
#include "camera.h"

#include <glm/glm.hpp>

#include <cmath>
#include <vector>

namespace GNC {

// A plane in the form dot(normal, p) + distance = 0.
// The normal points towards the inside of the frustum, so a positive signed
// distance means "in front of the plane", i.e. on the visible side.
struct Plane {
    glm::vec3 normal {0.0f, 1.0f, 0.0f};
    float distance = 0.0f;

    float SignedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

// The camera volume, described by the six planes that bound it.
struct Frustum {
    enum Side { Left, Right, Bottom, Top, Near, Far, SideCount };

    Plane planes[SideCount];

    // All six planes fall straight out of the rows of the view-projection
    // matrix (Gribb & Hartmann). Row i is (m[0][i], m[1][i], m[2][i], m[3][i])
    // because glm stores matrices column major.
    static Frustum FromViewProjection(const glm::mat4& m) {
        const glm::vec4 rowX {m[0][0], m[1][0], m[2][0], m[3][0]};
        const glm::vec4 rowY {m[0][1], m[1][1], m[2][1], m[3][1]};
        const glm::vec4 rowZ {m[0][2], m[1][2], m[2][2], m[3][2]};
        const glm::vec4 rowW {m[0][3], m[1][3], m[2][3], m[3][3]};

        Frustum frustum;
        frustum.planes[Left]   = MakePlane(rowW + rowX);
        frustum.planes[Right]  = MakePlane(rowW - rowX);
        frustum.planes[Bottom] = MakePlane(rowW + rowY);
        frustum.planes[Top]    = MakePlane(rowW - rowY);
        // glm defaults to the OpenGL depth range (-1..1). With
        // GLM_FORCE_DEPTH_ZERO_TO_ONE (Vulkan) the near plane is just rowZ.
        frustum.planes[Near]   = MakePlane(rowW + rowZ);
        frustum.planes[Far]    = MakePlane(rowW - rowZ);
        return frustum;
    }

    // Conservative test: false only when the box is provably outside.
    bool Intersects(const AABBBoundingBox3D& bbox) const {
        const glm::vec3 center = bbox.Center();
        const glm::vec3 extents = bbox.Extents();

        for (const Plane& plane : planes) {
            // How far the box reaches from its center along the plane normal.
            const float radius = extents.x * std::abs(plane.normal.x)
                               + extents.y * std::abs(plane.normal.y)
                               + extents.z * std::abs(plane.normal.z);

            // Even the corner closest to the inside is behind the plane.
            if (plane.SignedDistance(center) < -radius) return false;
        }
        return true;
    }

private:
    static Plane MakePlane(const glm::vec4& v) {
        const float length = glm::length(glm::vec3(v));
        return Plane{glm::vec3(v) / length, v.w / length};
    }
};

class FrustumCulling {
private:
    Camera* camera;
    std::vector<Entity*> visibleEntities;

public:
    explicit FrustumCulling(Camera* cam) : camera(cam) {}

    void SetCamera(Camera* cam) {
        camera = cam;
    }

    void CullScene(const std::vector<Entity*>& allEntities) {
        visibleEntities.clear();

        if (!camera) return;

        const Frustum frustum = Frustum::FromViewProjection(camera->Projection * camera->GetViewMatrix());

        for (auto entity : allEntities) {
            if (!entity->IsActive()) continue;

            auto meshComponent = entity->GetComponent<MeshComponent>();
            if (!meshComponent) continue;

            auto transformComponent = entity->GetComponent<TransformComponent>();
            if (!transformComponent) continue;

            // The mesh box is in model space, so move it into world space first:
            // the frustum planes live in world space too.
            AABBBoundingBox3D boundingBox = meshComponent->GetBoundingBox();
            boundingBox.Transform(transformComponent->GetTransformMatrix());

            if (frustum.Intersects(boundingBox)) {
                visibleEntities.push_back(entity);
            }
        }
    }

    const std::vector<Entity*>& GetVisibleEntities() const {
        return visibleEntities;
    }
};

} // namespace GNC
