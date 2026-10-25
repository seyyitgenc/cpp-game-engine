#include "CameraController.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Camera.hpp"
#include "Event/Events.hpp"
#include "Platform/Window.hpp"

namespace GNC {

namespace {

bool IsMovementKey(int key) {
    switch (key) {
        case GLFW_KEY_W: case GLFW_KEY_A: case GLFW_KEY_S: case GLFW_KEY_D:
        case GLFW_KEY_E: case GLFW_KEY_SPACE: case GLFW_KEY_Q: case GLFW_KEY_LEFT_CONTROL:
            return true;
        default:
            return false;
    }
}

} // namespace

CameraController::CameraController(EventBus& bus, Window& win, Camera& cam) : camera(cam), window(win) {
    // Tracking a key is not consuming it: listeners below (App's Esc binding,
    // the gizmo shortcuts) still need it. Only while looking do the movement
    // keys belong to the camera.
    subscriptions.push_back(bus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (!e.IsRepeat()) heldKeys.insert(e.GetKey());
        return looking && IsMovementKey(e.GetKey());
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<KeyReleasedEvent>([this](KeyReleasedEvent& e) {
        heldKeys.erase(e.GetKey());
        return false;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e) {
        if (e.GetButton() != GLFW_MOUSE_BUTTON_RIGHT) return false;
        SetLooking(true);
        return true;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& e) {
        if (e.GetButton() != GLFW_MOUSE_BUTTON_RIGHT) return false;
        SetLooking(false);
        return true;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseMovedEvent>([this](MouseMovedEvent& e) {
        if (!looking) return false;

        if (hasLastCursor) {
            // Screen Y grows downwards and pitch upwards, hence last - current.
            camera.ProcessMouseMovement(static_cast<float>(e.GetX() - lastCursorX),
                                        static_cast<float>(lastCursorY - e.GetY()));
        }
        lastCursorX = e.GetX();
        lastCursorY = e.GetY();
        hasLastCursor = true;
        return true;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<MouseScrolledEvent>([this](MouseScrolledEvent& e) {
        camera.ProcessMouseScroll(static_cast<float>(e.GetOffsetY()));
        return true;
    }, EventPriority));

    // Alt-tabbing away while looking would leave the cursor captured, and the
    // releases go to another window, so drop the held keys too.
    subscriptions.push_back(bus.Subscribe<WindowFocusEvent>([this](WindowFocusEvent& e) {
        if (!e.IsFocused()) {
            SetLooking(false);
            heldKeys.clear();
        }
        return false;
    }, EventPriority));
}

void CameraController::Update(float deltaTime) {
    // Otherwise the same keys pick gizmo tools.
    if (!looking) return;

    const float step = IsHeld(GLFW_KEY_LEFT_SHIFT) ? deltaTime * BoostMultiplier : deltaTime;

    if (IsHeld(GLFW_KEY_W)) camera.ProcessKeyboard(CameraMovement::Forward, step);
    if (IsHeld(GLFW_KEY_S)) camera.ProcessKeyboard(CameraMovement::Backward, step);
    if (IsHeld(GLFW_KEY_A)) camera.ProcessKeyboard(CameraMovement::Left, step);
    if (IsHeld(GLFW_KEY_D)) camera.ProcessKeyboard(CameraMovement::Right, step);
    if (IsHeld(GLFW_KEY_E) || IsHeld(GLFW_KEY_SPACE)) {
        camera.ProcessKeyboard(CameraMovement::Up, step);
    }
    if (IsHeld(GLFW_KEY_Q) || IsHeld(GLFW_KEY_LEFT_CONTROL)) {
        camera.ProcessKeyboard(CameraMovement::Down, step);
    }
}

void CameraController::SetLooking(bool look) {
    if (looking == look) return;

    looking = look;
    hasLastCursor = false;
    window.SetCursorCaptured(look);
}

} // namespace GNC
