#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Culling/BoundingVolume.hpp"

namespace GNC {

// Debug draw
// What DebugDrawPass draws over the scene in the next frame recorded, after
// which the Renderer clears it. Colors are sRGB encoded, as the scene image
// holds them.
class DebugDraw {
public:
    struct Volume {
        // Valid until the frame is recorded.
        const BoundingVolume* volume;
        // Changes whenever the volume does: the pass keeps a hull's mesh by it.
        std::uint64_t id;
        // Model space to world space.
        glm::mat4 model;
        glm::vec4 color;
    };

private:
    std::vector<Volume> volumes;

public:
    void AddVolume(const BoundingVolume& volume, std::uint64_t id, const glm::mat4& model, const glm::vec4& color) {
        volumes.push_back({ &volume, id, model, color });
    }

    const std::vector<Volume>& GetVolumes() const { return volumes; }
    void Clear() { volumes.clear(); }
};

} // namespace GNC
