#pragma once

#include <optional>

#include <glm/glm.hpp>

struct ImVec2;

namespace GNC {

class Camera;

// Orientation gizmo
// The world axes as the camera sees them, drawn into the current window
// around center, as Blender's and Godot's navigation gizmos are: X red, Y
// green, Z blue, the negative ends dimmer. It is an item of its own, so it
// takes the clicks over it. Returns, the frame an axis is clicked, the view
// direction to turn the camera to: toward the center from that axis's side,
// or from the other side if the camera already looks that way.
std::optional<glm::vec3> OrientationGizmo(const char* id, const Camera& camera, const ImVec2& center, float radius);

} // namespace GNC
