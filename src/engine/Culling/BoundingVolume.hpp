#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

namespace GNC {

// Bounding volumes
// Shapes that enclose a set of points, in the points' own space. IsBehind()
// is true when all of a shape lies on the negative side of the plane
// dot(normal, p) + distance = 0. The normal need not be unit length: a
// world-space plane moved into model space works as it is, and distances
// come out in world units.

// All six sides of a box as one triangle strip, by corner: corner i takes the
// maximum along each axis whose bit is set (1 x, 2 y, 4 z).
inline constexpr std::array<std::uint32_t, 14> BoxStrip { 6, 7, 4, 5, 1, 7, 3, 6, 2, 4, 0, 1, 2, 3 };

// Axis aligned bounding box, as its two opposite corners.
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

    static AABBBoundingBox3D FromPoints(std::span<const glm::vec3> points) {
        AABBBoundingBox3D box = Empty();
        for (const glm::vec3& point : points) {
            box.Grow(point);
        }
        return box;
    }

    void Grow(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    bool IsBehind(const glm::vec3& normal, float distance) const {
        // How far the box reaches from its center along the normal.
        const float radius = glm::dot(Extents(), glm::abs(normal));
        return glm::dot(normal, Center()) + distance < -radius;
    }
};

struct BoundingSphere {
    glm::vec3 center {0.0f};
    float radius = 1.0f;

    // Ritter's method: two passes, a little larger than the smallest sphere.
    static BoundingSphere FromPoints(std::span<const glm::vec3> points);

    bool IsBehind(const glm::vec3& normal, float distance) const {
        return glm::dot(normal, center) + distance < -radius * glm::length(normal);
    }
};

// The hull's corners: the points it was fitted to, less those inside it.
struct ConvexHull {
    std::vector<glm::vec3> vertices;
    // Of vertices, for drawing.
    std::vector<glm::uvec3> triangles;
    // How far rounding may leave a point outside the hull, in model units.
    float tolerance = 0.0f;

    // Quickhull. Flat input (a plane, a line) encloses no volume, so it gets
    // the corners of its box instead, as does input that rounding breaks the
    // hull on.
    static ConvexHull FromPoints(std::span<const glm::vec3> points);

    bool IsBehind(const glm::vec3& normal, float distance) const {
        const float slack = tolerance * glm::length(normal);
        return std::ranges::all_of(vertices, [&](const glm::vec3& vertex) { return glm::dot(normal, vertex) + distance < -slack; });
    }
};

using BoundingVolume = std::variant<AABBBoundingBox3D, BoundingSphere, ConvexHull>;

} // namespace GNC
