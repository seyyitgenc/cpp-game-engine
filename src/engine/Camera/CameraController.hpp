#pragma once

#include <optional>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "Event/EventBus.hpp"

namespace GNC {

class Camera;
class Window;

// Camera controller
// Editor-style fly camera: hold the right mouse button to look (the cursor is
// captured meanwhile) and, while it is held, WASD to move, E/Space up, Q/Left
// Ctrl down, Left Shift faster. The wheel zooms. Input comes from the bus, so
// ImGui can swallow it first. An AlignCameraEvent orbits it to a view
// direction, until looking takes over.
class CameraController {
public:
    static constexpr float BoostMultiplier = 4.0f;
    // Seconds an AlignCameraEvent's orbit takes.
    static constexpr float AlignDuration = 0.25f;

    // Above the Viewport's gizmo shortcuts, which share the movement keys.
    static constexpr int EventPriority = 100;

private:
    Camera& camera;
    Window& window;

    bool looking = false;
    // The first cursor event after capture only sets the reference point, or
    // the view would jump by however far the cursor moved while free.
    bool hasLastCursor = false;
    double lastCursorX = 0.0;
    double lastCursorY = 0.0;

    // A press swallowed by a higher priority listener never lands here.
    std::unordered_set<int> heldKeys;

    // In yaw and pitch, so the camera never rolls on the way. The orbit's
    // center moves from straight ahead of the camera to the pivot, so the
    // first frame does not jump when the pivot is off center.
    struct Alignment {
        glm::vec3 startCenter;
        glm::vec3 pivot;
        float distance;
        float startYaw;
        float startPitch;
        float yawChange; // degrees, the short way round
        float targetPitch;
        float elapsed = 0.0f;
    };
    std::optional<Alignment> alignment;

    std::vector<EventSubscription> subscriptions;

public:
    CameraController(EventBus& bus, Window& win, Camera& cam);

    // The subscriptions capture this, so the controller stays put.
    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;

    // deltaTime in seconds.
    void Update(float deltaTime);

private:
    void SetLooking(bool look);
    void Align(const glm::vec3& viewDirection, const glm::vec3& pivot);
    void Animate(float deltaTime);
    bool IsHeld(int key) const { return heldKeys.contains(key); }
};

} // namespace GNC
