#include "BoundingVolume.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>

namespace GNC {

namespace {

float LengthSquared(const glm::vec3& vector) { return glm::dot(vector, vector); }
double LengthSquared(const glm::dvec3& vector) { return glm::dot(vector, vector); }

// -----------
// Convex Hull
// -----------
constexpr std::uint32_t NoFace = std::numeric_limits<std::uint32_t>::max();

struct HullFace {
    // Counter-clockwise seen from outside.
    std::array<std::uint32_t, 3> vertices {};
    // neighbors[i] shares the edge from vertices[i] to vertices[(i + 1) % 3].
    std::array<std::uint32_t, 3> neighbors { NoFace, NoFace, NoFace };
    glm::dvec3 normal {0.0};
    double offset = 0.0;
    // Points above this face, not yet on the hull.
    std::vector<std::uint32_t> outside {};
    bool removed = false;
    // Seen from the point being added.
    bool visible = false;
};

struct HorizonEdge {
    std::uint32_t from;
    std::uint32_t to;
    // The face beyond the edge, which the point does not see.
    std::uint32_t outer;
};

// Quickhull (Barber, Dobkin and Huhdanpaa), in double precision. A point
// counts as above a face only when it is more than epsilon above it. Faces
// are never merged, so where points are nearly coplanar the hull can fold
// inwards a little, and a point can end up outside it; ConvexHull keeps
// those points as corners too.
class Quickhull {
private:
    std::vector<glm::dvec3> points;
    double epsilon = 0.0;
    std::vector<HullFace> faces;
    bool broken = false;

public:
    explicit Quickhull(std::span<const glm::vec3> input) : points(input.begin(), input.end()) {
        glm::dvec3 largest(0.0);
        for (const glm::dvec3& point : points) {
            largest = glm::max(largest, glm::abs(point));
        }
        // The input is float: points this close to a plane count as on it.
        epsilon = 3.0 * std::numeric_limits<float>::epsilon() * (largest.x + largest.y + largest.z);
    }

    double GetEpsilon() const { return epsilon; }

    // The hull's corners, as indices into the input. Nothing for flat input,
    // or when rounding broke the hull.
    std::optional<std::vector<std::uint32_t>> Build() {
        if (!AddSimplex()) return std::nullopt;

        // Faces appended while this runs are reached in turn. A face never
        // gains outside points after it is made, so one pass empties them all.
        for (std::uint32_t index = 0; index < faces.size() && !broken; ++index) {
            while (!broken && !faces[index].removed && !faces[index].outside.empty()) {
                AddPoint(index);
            }
        }
        if (broken || !IsClosed()) return std::nullopt;

        std::vector<std::uint32_t> corners;
        for (const HullFace& face : faces) {
            if (!face.removed) corners.insert(corners.end(), face.vertices.begin(), face.vertices.end());
        }
        std::ranges::sort(corners);
        corners.erase(std::ranges::unique(corners).begin(), corners.end());
        return corners;
    }

    // As indices into the input, after Build().
    std::vector<glm::uvec3> GetTriangles() const {
        std::vector<glm::uvec3> triangles;
        for (const HullFace& face : faces) {
            if (!face.removed) triangles.emplace_back(face.vertices[0], face.vertices[1], face.vertices[2]);
        }
        return triangles;
    }

    bool IsOutside(std::uint32_t point) const {
        for (const HullFace& face : faces) {
            if (!face.removed && Distance(face, points[point]) > epsilon) return true;
        }
        return false;
    }

private:
    static double Distance(const HullFace& face, const glm::dvec3& point) {
        return glm::dot(face.normal, point) - face.offset;
    }

    template<typename Measure>
    std::uint32_t FarthestBy(Measure measure) const {
        std::uint32_t farthest = 0;
        double largest = -1.0;
        for (std::uint32_t i = 0; i < points.size(); ++i) {
            const double value = measure(points[i]);
            if (value > largest) {
                largest = value;
                farthest = i;
            }
        }
        return farthest;
    }

    // A tetrahedron of four points far apart, with every point above one of
    // its faces assigned to that face.
    bool AddSimplex() {
        if (points.size() < 4) return false;

        // Of the extreme points along each axis, the two farthest apart.
        std::array<std::uint32_t, 6> extremes {};
        for (std::uint32_t i = 0; i < points.size(); ++i) {
            for (int axis = 0; axis < 3; ++axis) {
                if (points[i][axis] < points[extremes[axis * 2]][axis]) extremes[axis * 2] = i;
                if (points[i][axis] > points[extremes[axis * 2 + 1]][axis]) extremes[axis * 2 + 1] = i;
            }
        }
        std::uint32_t a = 0;
        std::uint32_t b = 0;
        double farthest = 0.0;
        for (const std::uint32_t i : extremes) {
            for (const std::uint32_t j : extremes) {
                const double distance = LengthSquared(points[i] - points[j]);
                if (distance > farthest) {
                    farthest = distance;
                    a = i;
                    b = j;
                }
            }
        }
        if (std::sqrt(farthest) <= epsilon) return false;

        // Then the point farthest from the line through them, and the one
        // farthest from the plane through all three.
        const glm::dvec3 direction = glm::normalize(points[b] - points[a]);
        std::uint32_t c = FarthestBy([&](const glm::dvec3& point) { return LengthSquared(glm::cross(point - points[a], direction)); });
        if (glm::length(glm::cross(points[c] - points[a], direction)) <= epsilon) return false;

        const glm::dvec3 normal = glm::normalize(glm::cross(points[b] - points[a], points[c] - points[a]));
        const std::uint32_t d = FarthestBy([&](const glm::dvec3& point) { return std::abs(glm::dot(normal, point - points[a])); });
        const double height = glm::dot(normal, points[d] - points[a]);
        if (std::abs(height) <= epsilon) return false;
        // The base faces away from d.
        if (height > 0.0) std::swap(b, c);

        // Each side takes one edge of the base, the other way round.
        AddFace(a, b, c);
        AddFace(b, a, d);
        AddFace(c, b, d);
        AddFace(a, c, d);
        faces[0].neighbors = { 1, 2, 3 };
        faces[1].neighbors = { 0, 3, 2 };
        faces[2].neighbors = { 0, 1, 3 };
        faces[3].neighbors = { 0, 2, 1 };

        for (std::uint32_t i = 0; i < points.size(); ++i) {
            AssignOutside(i, 0, 4);
        }
        return !broken;
    }

    void AddFace(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
        HullFace face { .vertices = { a, b, c } };
        const glm::dvec3 cross = glm::cross(points[b] - points[a], points[c] - points[a]);
        const double length = glm::length(cross);
        if (length > 0.0) {
            face.normal = cross / length;
        } else {
            broken = true;
        }
        face.offset = glm::dot(face.normal, (points[a] + points[b] + points[c]) / 3.0);
        faces.push_back(std::move(face));
    }

    // To the face among [first, last) the point is highest above, if any.
    void AssignOutside(std::uint32_t point, std::uint32_t first, std::uint32_t last) {
        std::uint32_t highestFace = NoFace;
        double highest = epsilon;
        for (std::uint32_t index = first; index < last; ++index) {
            const double distance = Distance(faces[index], points[point]);
            if (distance > highest) {
                highest = distance;
                highestFace = index;
            }
        }
        if (highestFace != NoFace) faces[highestFace].outside.push_back(point);
    }

    std::optional<std::uint32_t> EdgeIndex(std::uint32_t face, std::uint32_t from, std::uint32_t to) const {
        const std::array<std::uint32_t, 3>& vertices = faces[face].vertices;
        for (std::uint32_t i = 0; i < 3; ++i) {
            if (vertices[i] == from && vertices[(i + 1) % 3] == to) return i;
        }
        return std::nullopt;
    }

    // Puts the face's point farthest out on the hull: the faces it sees
    // make way for a cone of new faces from their outline up to it.
    void AddPoint(std::uint32_t faceIndex) {
        const HullFace& face = faces[faceIndex];
        const std::uint32_t eye = *std::ranges::max_element(face.outside, {}, [&](std::uint32_t point) { return Distance(face, points[point]); });

        std::vector<std::uint32_t> visible;
        std::vector<HorizonEdge> horizon;
        if (!FindHorizon(faceIndex, points[eye], visible, horizon)) {
            // Where the hull folds, the faces the eye sees need not have one
            // outline. The eye stays off the hull and outside it.
            for (const std::uint32_t index : visible) {
                faces[index].visible = false;
            }
            std::erase(faces[faceIndex].outside, eye);
            return;
        }

        const auto first = static_cast<std::uint32_t>(faces.size());
        const auto count = static_cast<std::uint32_t>(horizon.size());
        for (std::uint32_t k = 0; k < count; ++k) {
            const HorizonEdge& edge = horizon[k];
            AddFace(edge.from, edge.to, eye);
            faces.back().neighbors = { edge.outer, first + (k + 1) % count, first + (k + count - 1) % count };

            const std::optional<std::uint32_t> back = EdgeIndex(edge.outer, edge.to, edge.from);
            if (!back) {
                broken = true;
                return;
            }
            faces[edge.outer].neighbors[*back] = first + k;
        }

        // Points above the faces the eye saw are above a new face, or inside.
        for (const std::uint32_t index : visible) {
            for (const std::uint32_t point : faces[index].outside) {
                if (point != eye) AssignOutside(point, first, first + count);
            }
            faces[index].outside = {};
            faces[index].removed = true;
        }
    }

    // The faces the eye is above, found by walking across edges from start,
    // and the loop of edges around them in order: a depth-first walk that
    // carries on around each face from the edge after the one it came in by.
    bool FindHorizon(std::uint32_t start, const glm::dvec3& eye, std::vector<std::uint32_t>& visible, std::vector<HorizonEdge>& horizon) {
        struct Step {
            std::uint32_t face;
            std::uint32_t edge;
            std::uint32_t remaining;
        };
        std::vector<Step> stack { { start, 0, 3 } };
        faces[start].visible = true;
        visible.push_back(start);

        while (!stack.empty()) {
            Step& step = stack.back();
            if (step.remaining == 0) {
                stack.pop_back();
                continue;
            }
            const std::uint32_t face = step.face;
            const std::uint32_t edge = step.edge;
            step.edge = (edge + 1) % 3;
            --step.remaining;

            const std::uint32_t from = faces[face].vertices[edge];
            const std::uint32_t to = faces[face].vertices[(edge + 1) % 3];
            const std::uint32_t neighbor = faces[face].neighbors[edge];
            if (neighbor == NoFace || faces[neighbor].removed) return false;
            if (faces[neighbor].visible) continue;

            if (Distance(faces[neighbor], eye) > epsilon) {
                const std::optional<std::uint32_t> back = EdgeIndex(neighbor, to, from);
                if (!back) return false;
                faces[neighbor].visible = true;
                visible.push_back(neighbor);
                stack.push_back({ neighbor, (*back + 1) % 3, 2 });
            } else {
                horizon.push_back({ from, to, neighbor });
            }
        }

        // One simple loop, or the new faces would not close up.
        if (horizon.size() < 3) return false;
        std::vector<std::uint32_t> loop;
        for (std::size_t k = 0; k < horizon.size(); ++k) {
            if (horizon[k].to != horizon[(k + 1) % horizon.size()].from) return false;
            loop.push_back(horizon[k].from);
        }
        std::ranges::sort(loop);
        return std::ranges::adjacent_find(loop) == loop.end();
    }

    // Every edge has a face on each side that agrees it does. A point above
    // no face of a closed hull is inside it, however the hull folds.
    bool IsClosed() const {
        for (std::uint32_t index = 0; index < faces.size(); ++index) {
            const HullFace& face = faces[index];
            if (face.removed) continue;

            for (std::uint32_t edge = 0; edge < 3; ++edge) {
                const std::uint32_t neighbor = face.neighbors[edge];
                if (neighbor == NoFace || faces[neighbor].removed) return false;

                const std::optional<std::uint32_t> back = EdgeIndex(neighbor, face.vertices[(edge + 1) % 3], face.vertices[edge]);
                if (!back || faces[neighbor].neighbors[*back] != index) return false;
            }
        }
        return true;
    }
};

} // namespace

BoundingSphere BoundingSphere::FromPoints(std::span<const glm::vec3> points) {
    if (points.empty()) return {};

    // Start from two points far apart: the farthest from any point, and the
    // farthest from that.
    const glm::vec3 first = *std::ranges::max_element(points, {}, [&](const glm::vec3& point) { return LengthSquared(point - points[0]); });
    const glm::vec3 second = *std::ranges::max_element(points, {}, [&](const glm::vec3& point) { return LengthSquared(point - first); });

    BoundingSphere sphere { .center = (first + second) * 0.5f, .radius = glm::length(second - first) * 0.5f };
    for (const glm::vec3& point : points) {
        const float distance = glm::length(point - sphere.center);
        if (distance <= sphere.radius) continue;

        // The smallest sphere around both the old one and the point.
        const float radius = (sphere.radius + distance) * 0.5f;
        sphere.center += (point - sphere.center) * ((radius - sphere.radius) / distance);
        sphere.radius = radius;
    }

    // Each step above can leave the points before it a rounding error
    // outside.
    float radiusSquared = 0.0f;
    for (const glm::vec3& point : points) {
        radiusSquared = std::max(radiusSquared, LengthSquared(point - sphere.center));
    }
    sphere.radius = std::sqrt(radiusSquared);
    return sphere;
}

BoundingCapsule BoundingCapsule::FromPoints(std::span<const glm::vec3> points) {
    if (points.empty()) return {};

    // ----
    // Axis
    // ----
    // The way the points spread furthest: the covariance's dominant
    // eigenvector, by power iteration. A slow convergence only costs
    // tightness, since the rest fits any axis.
    glm::dvec3 mean(0.0);
    for (const glm::vec3& point : points) {
        mean += glm::dvec3(point);
    }
    mean /= static_cast<double>(points.size());

    glm::dmat3 covariance(0.0);
    for (const glm::vec3& point : points) {
        const glm::dvec3 offset = glm::dvec3(point) - mean;
        covariance += glm::outerProduct(offset, offset);
    }
    // Started from its longest column, which is zero only when the points
    // are all one point.
    glm::dvec3 axis = covariance[0];
    for (int column = 1; column < 3; ++column) {
        if (LengthSquared(covariance[column]) > LengthSquared(axis)) axis = covariance[column];
    }
    if (LengthSquared(axis) == 0.0) {
        axis = glm::dvec3(0.0, 1.0, 0.0);
    } else {
        for (int iteration = 0; iteration < 32; ++iteration) {
            axis = glm::normalize(covariance * axis);
        }
    }

    // ------
    // Radius
    // ------
    // The line along the axis through the center of the smallest circle
    // around the points, seen down the axis.
    std::vector<glm::vec3> flattened;
    flattened.reserve(points.size());
    for (const glm::vec3& point : points) {
        const glm::dvec3 position(point);
        flattened.emplace_back(position - axis * glm::dot(position, axis));
    }
    const glm::dvec3 center(BoundingSphere::FromPoints(flattened).center);

    double radiusSquared = 0.0;
    for (const glm::vec3& point : points) {
        const glm::dvec3 offset = glm::dvec3(point) - center;
        radiusSquared = std::max(radiusSquared, LengthSquared(offset - axis * glm::dot(offset, axis)));
    }

    // ----
    // Ends
    // ----
    // A point off the segment's end is inside if the end is within the
    // height of the round end above the point's distance from the axis.
    double first = std::numeric_limits<double>::max();
    double last = std::numeric_limits<double>::lowest();
    for (const glm::vec3& point : points) {
        const glm::dvec3 offset = glm::dvec3(point) - center;
        const double along = glm::dot(offset, axis);
        const double cap = std::sqrt(std::max(radiusSquared - LengthSquared(offset - axis * along), 0.0));
        first = std::min(first, along + cap);
        last = std::max(last, along - cap);
    }
    // Short enough for one sphere: any point between the two does.
    if (first > last) first = last = (first + last) * 0.5;

    return BoundingCapsule {
        .start = glm::vec3(center + axis * first),
        .end = glm::vec3(center + axis * last),
        .radius = static_cast<float>(std::sqrt(radiusSquared))
    };
}

ConvexHull ConvexHull::FromPoints(std::span<const glm::vec3> points) {
    Quickhull quickhull(points);
    const std::optional<std::vector<std::uint32_t>> corners = quickhull.Build();

    ConvexHull hull;
    if (!corners) {
        const AABBBoundingBox3D box = AABBBoundingBox3D::FromPoints(points);
        for (std::uint32_t corner = 0; corner < 8; ++corner) {
            hull.vertices.emplace_back(corner & 1 ? box.max.x : box.min.x, corner & 2 ? box.max.y : box.min.y, corner & 4 ? box.max.z : box.min.z);
        }
        for (std::size_t i = 0; i + 2 < BoxStrip.size(); ++i) {
            hull.triangles.emplace_back(BoxStrip[i], BoxStrip[i + 1], BoxStrip[i + 2]);
        }
        return hull;
    }

    constexpr std::uint32_t NotCorner = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::uint32_t> vertexOf(points.size(), NotCorner);
    for (const std::uint32_t corner : *corners) {
        vertexOf[corner] = static_cast<std::uint32_t>(hull.vertices.size());
        hull.vertices.push_back(points[corner]);
    }
    for (const glm::uvec3& triangle : quickhull.GetTriangles()) {
        hull.triangles.emplace_back(vertexOf[triangle.x], vertexOf[triangle.y], vertexOf[triangle.z]);
    }
    // Points the hull left outside it (see Quickhull).
    for (std::uint32_t i = 0; i < points.size(); ++i) {
        if (vertexOf[i] == NotCorner && quickhull.IsOutside(i)) hull.vertices.push_back(points[i]);
    }
    // A point epsilon above two faces at a sharp edge is farther than
    // epsilon from the hull.
    hull.tolerance = static_cast<float>(4.0 * quickhull.GetEpsilon());
    return hull;
}

} // namespace GNC
