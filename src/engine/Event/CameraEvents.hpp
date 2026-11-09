#pragma once

#include "Event.hpp"

#include <glm/glm.hpp>

namespace GNC {

// Turns the editor camera to look along viewDirection at pivot, keeping its
// distance from pivot: an orbit, which CameraController animates. Sent by
// the Viewport's orientation gizmo.
class AlignCameraEvent : public Event {
private:
    glm::vec3 viewDirection;
    glm::vec3 pivot;

public:
    GNC_EVENT_TYPE(AlignCameraEvent, EventCategory::Application)

    // viewDirection need not be normalized, but must not be zero.
    AlignCameraEvent(const glm::vec3& direction, const glm::vec3& center) : viewDirection(direction), pivot(center) {}

    const glm::vec3& GetViewDirection() const { return viewDirection; }
    const glm::vec3& GetPivot() const { return pivot; }
};

} // namespace GNC
