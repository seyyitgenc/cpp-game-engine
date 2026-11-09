#include "CameraController.hpp"

#include <algorithm>
#include <cmath>

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

        if (hasLastCursor && twoD) {
            Pan(e.GetX() - lastCursorX, e.GetY() - lastCursorY);
        } else if (hasLastCursor) {
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
        if (twoD) {
            const float halfHeight = camera.GetOrthographicHalfHeight() * std::pow(ZoomStep, static_cast<float>(e.GetOffsetY()));
            camera.SetOrthographicHalfHeight(std::clamp(halfHeight, MinHalfHeight, MaxHalfHeight));
        } else {
            camera.ProcessMouseScroll(static_cast<float>(e.GetOffsetY()));
        }
        return true;
    }, EventPriority));

    subscriptions.push_back(bus.Subscribe<AlignCameraEvent>([this](AlignCameraEvent& e) {
        Align(e.GetViewDirection(), e.GetPivot());
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
    if (alignment) Animate(deltaTime);

    // Otherwise the same keys pick gizmo tools.
    if (!looking) return;

    const float step = IsHeld(GLFW_KEY_LEFT_SHIFT) ? deltaTime * BoostMultiplier : deltaTime;

    // Within the plane, at a speed that follows the zoom.
    if (twoD) {
        const glm::vec2 direction((IsHeld(GLFW_KEY_D) ? 1.0f : 0.0f) - (IsHeld(GLFW_KEY_A) ? 1.0f : 0.0f),
                                  (IsHeld(GLFW_KEY_W) ? 1.0f : 0.0f) - (IsHeld(GLFW_KEY_S) ? 1.0f : 0.0f));
        const float distance = camera.GetOrthographicHalfHeight() * 2.0f * PanSpeed * step;
        camera.SetPosition(camera.GetPosition() + (camera.GetRight() * direction.x + camera.GetUp() * direction.y) * distance);
        return;
    }

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
    // The user takes over.
    if (look) alignment.reset();
}

void CameraController::SetTwoD(bool twoDScene) {
    twoD = twoDScene;
    alignment.reset();
}

// A 2D camera does not turn.
void CameraController::Align(const glm::vec3& viewDirection, const glm::vec3& pivot) {
    if (twoD || looking || glm::dot(viewDirection, viewDirection) <= 0.0f) return;

    const glm::vec3 direction = glm::normalize(viewDirection);
    // Straight up or down has no heading of its own: keep the current one.
    const float targetYaw = std::abs(direction.y) > 0.999f ? camera.GetYaw() : glm::degrees(std::atan2(direction.z, direction.x));
    const float distance = glm::length(pivot - camera.GetPosition());

    alignment = Alignment {
        .startCenter = camera.GetPosition() + camera.GetFront() * distance,
        .pivot = pivot,
        .distance = distance,
        .startYaw = camera.GetYaw(),
        .startPitch = camera.GetPitch(),
        .yawChange = std::remainder(targetYaw - camera.GetYaw(), 360.0f),
        .targetPitch = glm::degrees(std::asin(std::clamp(direction.y, -1.0f, 1.0f))),
    };
}

void CameraController::Animate(float deltaTime) {
    alignment->elapsed += deltaTime;
    const float t = std::min(alignment->elapsed / AlignDuration, 1.0f);
    const float eased = t * t * (3.0f - 2.0f * t);

    // SetRotation keeps the pitch off the poles, and the position follows
    // the front it actually got.
    camera.SetRotation(alignment->startYaw + alignment->yawChange * eased, glm::mix(alignment->startPitch, alignment->targetPitch, eased));
    const glm::vec3 center = glm::mix(alignment->startCenter, alignment->pivot, eased);
    camera.SetPosition(center - camera.GetFront() * alignment->distance);

    if (t >= 1.0f) alignment.reset();
}

// Screen y grows downwards. One pixel is the view's height over the
// viewport's: GLFW's cursor coordinates are pixels on Windows.
void CameraController::Pan(double deltaX, double deltaY) {
    const float metersPerPixel = camera.GetOrthographicHalfHeight() * 2.0f / camera.GetViewportHeight();
    const glm::vec3 offset = camera.GetRight() * static_cast<float>(-deltaX) + camera.GetUp() * static_cast<float>(deltaY);
    camera.SetPosition(camera.GetPosition() + offset * metersPerPixel);
}

} // namespace GNC
