#pragma once

#include <unordered_set>
#include <vector>

#include "Event/EventBus.hpp"

namespace GNC {

class Camera;
class Window;

// Camera controller
// Fly camera driven by input, editor style:
//   - hold the right mouse button to look around (the cursor is captured
//     while it is held)
//   - WASD move, E / Space up, Q / Left Ctrl down, Left Shift for speed
//   - mouse wheel zooms (changes the field of view)
//
// All input arrives as events through the bus, so a higher priority listener,
// say an ImGui layer, can swallow it before the camera sees it. Held keys are
// collected from KeyPressed/KeyReleased; Update() moves by them once per frame.
class CameraController {
public:
    static constexpr float BoostMultiplier = 4.0f;

private:
    Camera& camera;
    Window& window;

    bool looking = false;
    // The first cursor event after capture only sets the reference point;
    // using it as a delta would jump the view by however far the cursor was
    // moved while it was free.
    bool hasLastCursor = false;
    double lastCursorX = 0.0;
    double lastCursorY = 0.0;

    // GLFW key codes currently down, as far as the events this controller
    // received say. A press swallowed by a higher priority listener never
    // lands here, which is the point.
    std::unordered_set<int> heldKeys;

    // Dropping these unsubscribes, so the listeners (which capture this)
    // cannot outlive the controller.
    std::vector<EventSubscription> subscriptions;

public:
    CameraController(EventBus& bus, Window& win, Camera& cam);

    // The subscriptions capture this, so the controller stays put.
    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;

    // Applies held movement keys. deltaTime in seconds.
    void Update(float deltaTime);

private:
    void SetLooking(bool look);
    bool IsHeld(int key) const { return heldKeys.contains(key); }
};

} // namespace GNC
