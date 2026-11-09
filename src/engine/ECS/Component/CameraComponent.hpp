#pragma once

#include "Component.hpp"
#include "Property.hpp"

#include <algorithm>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace GNC {

enum class CameraProjection : std::uint32_t {
    Perspective,
    Orthographic
};

// Camera component
// A camera of the scene's, at its entity's transform and looking down the
// local -Z axis with +Y up, as lights do: glTF's camera, with glTF's names.
// Only data for now: the editor's own Camera starts at the scene's camera
// when a scene opens. Both projections' values are kept, so switching back
// and forth loses nothing. The aspect ratio comes from the viewport.
class CameraComponent : public Component {
private:
    CameraProjection projection = CameraProjection::Perspective;
    float yfov = glm::radians(45.0f); // vertical, radians
    float ymag = 5.0f;                // half the orthographic view's height
    float znear = 0.1f;
    float zfar = 1000.0f;

public:
    GNC_COMPONENT_TYPE(CameraComponent, "Camera", "Camera")

    CameraComponent() = default;
    explicit CameraComponent(CameraProjection cameraProjection) : projection(cameraProjection) {}

    void SetProjection(CameraProjection value) { projection = value; }
    void SetYFov(float radians) { yfov = std::clamp(radians, glm::radians(1.0f), glm::radians(179.0f)); }
    void SetYMag(float halfHeight) { ymag = std::max(halfHeight, 0.001f); }
    void SetClipPlanes(float nearZ, float farZ) {
        znear = std::max(nearZ, 0.0001f);
        zfar = std::max(farZ, znear * 1.001f);
    }

    CameraProjection GetProjection() const { return projection; }
    float GetYFov() const { return yfov; }
    float GetYMag() const { return ymag; }
    float GetZNear() const { return znear; }
    float GetZFar() const { return zfar; }

    static void Reflect(PropertyList<CameraComponent>& properties) {
        properties.Add<int>(
            // CameraProjection's order.
            { .name = "Projection", .key = "projection", .hint = PropertyHint::Enum,
              .enumNames = { "Perspective", "Orthographic" }, .enumKeys = { "Perspective", "Orthographic" } },
            [](const CameraComponent& camera) { return static_cast<int>(camera.GetProjection()); },
            [](CameraComponent& camera, int value) { camera.SetProjection(static_cast<CameraProjection>(value)); });

        properties.Add<float>(
            { .name = "Field of View", .key = "yfov", .hint = PropertyHint::Angle, .min = glm::radians(1.0f), .max = glm::radians(179.0f) },
            [](const CameraComponent& camera) { return camera.GetYFov(); },
            [](CameraComponent& camera, float value) { camera.SetYFov(value); },
            [](const CameraComponent& camera, PropertyInfo& info) { info.visible = camera.GetProjection() == CameraProjection::Perspective; });

        properties.Add<float>(
            { .name = "Half Height", .key = "ymag", .min = 0.001f, .step = 0.05f, .format = "%.2f" },
            [](const CameraComponent& camera) { return camera.GetYMag(); },
            [](CameraComponent& camera, float value) { camera.SetYMag(value); },
            [](const CameraComponent& camera, PropertyInfo& info) { info.visible = camera.GetProjection() == CameraProjection::Orthographic; });

        // One setter for both keeps far beyond near; a larger near pushes far
        // along, so a scene file can set near first.
        properties.Add<float>(
            { .name = "Near", .key = "znear", .min = 0.0001f, .step = 0.01f, .format = "%.3f" },
            [](const CameraComponent& camera) { return camera.GetZNear(); },
            [](CameraComponent& camera, float value) { camera.SetClipPlanes(value, camera.GetZFar()); });

        properties.Add<float>(
            { .name = "Far", .key = "zfar", .min = 0.001f, .proportional = true, .format = "%.1f" },
            [](const CameraComponent& camera) { return camera.GetZFar(); },
            [](CameraComponent& camera, float value) { camera.SetClipPlanes(camera.GetZNear(), value); });
    }
};

} // namespace GNC
