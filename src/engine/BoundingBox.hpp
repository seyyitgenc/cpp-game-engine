#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <limits>

namespace GNC {

// Axis aligned bounding box, stored as its two opposite corners.
// Axis aligned means the edges always stay parallel to the world axes, so a box
// only ever needs these six numbers no matter how the object inside it is posed.
struct AABBBoundingBox3D {
    glm::vec3 min {-1.0f, -1.0f, -1.0f};
    glm::vec3 max { 1.0f,  1.0f,  1.0f};

    glm::vec3 Center() const { return (min + max) * 0.5f; }

    // Half the size along each axis, measured from the center.
    glm::vec3 Extents() const { return (max - min) * 0.5f; }

    // Start from an inverted box so the first Grow() snaps onto the point.
    static AABBBoundingBox3D Empty() {
        const float inf = std::numeric_limits<float>::max();
        return AABBBoundingBox3D{glm::vec3(inf), glm::vec3(-inf)};
    }

    void Grow(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    // A rotated box is no longer axis aligned, so the box is rebuilt around the
    // eight transformed corners. The result is a bit looser than the original.
    void Transform(const glm::mat4& transform) {
        const glm::vec3 oldMin = min;
        const glm::vec3 oldMax = max;

        const glm::vec3 corners[8] = {
            {oldMin.x, oldMin.y, oldMin.z},
            {oldMax.x, oldMin.y, oldMin.z},
            {oldMin.x, oldMax.y, oldMin.z},
            {oldMax.x, oldMax.y, oldMin.z},
            {oldMin.x, oldMin.y, oldMax.z},
            {oldMax.x, oldMin.y, oldMax.z},
            {oldMin.x, oldMax.y, oldMax.z},
            {oldMax.x, oldMax.y, oldMax.z},
        };

        *this = Empty();
        for (const glm::vec3& corner : corners) {
            Grow(glm::vec3(transform * glm::vec4(corner, 1.0f)));
        }
    }
};

} // namespace GNC
