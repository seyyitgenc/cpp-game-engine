#pragma once

#include "../ECS/Entity/Entity.hpp"

#include "../ECS/Component/BoundingVolumeComponent.hpp"
#include "../ECS/Component/MeshComponent.hpp"
#include "../ECS/Component/TransformsComponent.hpp"

#include "../Camera/Camera.hpp"

#include "BoundingVolume.hpp"

#include <glm/glm.hpp>

#include <variant>
#include <vector>

namespace GNC {

// dot(normal, p) + distance = 0, the normal pointing into the frustum: a
// positive signed distance is the visible side.
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
        // Vulkan's 0..1 depth makes the near plane just rowZ (OpenGL's would
        // be rowW + rowZ). The Y flip only swaps Bottom and Top.
        frustum.planes[Near]   = MakePlane(rowZ);
        frustum.planes[Far]    = MakePlane(rowW - rowZ);
        return frustum;
    }

    // Conservative test: false only when the volume is provably outside.
    // The planes move into the volume's model space rather than the volume
    // into world space, so the test stays exact under rotation, shear and
    // uneven scale. model must be affine.
    bool Intersects(const BoundingVolume& volume, const glm::mat4& model) const {
        const glm::mat3 linearTranspose = glm::transpose(glm::mat3(model));
        const glm::vec3 translation(model[3]);

        for (const Plane& plane : planes) {
            // dot(n, M p + t) + d = dot(transpose(M) n, p) + dot(n, t) + d
            const glm::vec3 normal = linearTranspose * plane.normal;
            const float distance = plane.SignedDistance(translation);
            if (std::visit([&](const auto& shape) { return shape.IsBehind(normal, distance); }, volume)) return false;
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

        const Frustum frustum = Frustum::FromViewProjection(camera->GetViewProjectionMatrix());

        for (auto entity : allEntities) {
            if (!entity->IsActiveInHierarchy()) continue;

            auto meshComponent = entity->GetComponent<MeshComponent>();
            if (!meshComponent) continue;

            auto transformComponent = entity->GetComponent<TransformComponent>();
            if (!transformComponent) continue;

            // Without a volume, it could be anywhere.
            auto boundsComponent = entity->GetComponent<BoundingVolumeComponent>();
            const BoundingVolume* volume = boundsComponent && boundsComponent->IsActive() ? boundsComponent->GetVolume() : nullptr;
            if (!volume || frustum.Intersects(*volume, transformComponent->GetWorldMatrix())) {
                visibleEntities.push_back(entity);
            }
        }
    }

    const std::vector<Entity*>& GetVisibleEntities() const {
        return visibleEntities;
    }

    // For the Renderer, which orders it for the outlines.
    std::vector<Entity*>& GetVisibleEntities() {
        return visibleEntities;
    }
};

} // namespace GNC
