#include "OrientationGizmo.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

#include <imgui.h>

#include "Camera/Camera.hpp"

namespace GNC {

namespace {

struct Handle {
    glm::vec3 axis;
    std::size_t axisIndex;
    ImVec2 position;
    // View space z: larger is nearer the viewer.
    float depth;
    ImU32 color;
    const char* label;
    bool positive;
};

// Blender's axis colors; ImGui colors are sRGB.
constexpr ImU32 AxisColors[3] = { IM_COL32(255, 51, 82, 255), IM_COL32(139, 220, 0, 255), IM_COL32(40, 144, 255, 255) };
constexpr const char* PositiveLabels[3] = { "X", "Y", "Z" };
constexpr const char* NegativeLabels[3] = { "-X", "-Y", "-Z" };

ImU32 Dimmed(ImU32 color) {
    ImVec4 rgba = ImGui::ColorConvertU32ToFloat4(color);
    rgba.x *= 0.45f;
    rgba.y *= 0.45f;
    rgba.z *= 0.45f;
    return ImGui::ColorConvertFloat4ToU32(rgba);
}

float DistanceSquared(const ImVec2& a, const ImVec2& b) {
    const float x = a.x - b.x;
    const float y = a.y - b.y;
    return x * x + y * y;
}

} // namespace

std::optional<glm::vec3> OrientationGizmo(const char* id, const Camera& camera, const ImVec2& center, float radius) {
    const float handleRadius = radius * 0.2f;
    const float axisLength = radius - handleRadius;

    // ----------------
    // Project the Axes
    // ----------------
    // The view's rotation alone: where each axis points on screen, and how
    // near. Screen y grows downwards.
    const glm::mat3 view(camera.GetViewMatrix());
    std::array<Handle, 6> handles;
    for (std::size_t i = 0; i < 6; ++i) {
        const std::size_t axisIndex = i % 3;
        const bool positive = i < 3;
        glm::vec3 axis(0.0f);
        axis[static_cast<glm::length_t>(axisIndex)] = positive ? 1.0f : -1.0f;

        const glm::vec3 seen = view * axis;
        handles[i] = Handle {
            .axis = axis,
            .axisIndex = axisIndex,
            .position = ImVec2(center.x + seen.x * axisLength, center.y - seen.y * axisLength),
            .depth = seen.z,
            .color = positive ? AxisColors[axisIndex] : Dimmed(AxisColors[axisIndex]),
            .label = positive ? PositiveLabels[axisIndex] : NegativeLabels[axisIndex],
            .positive = positive,
        };
    }
    // Far to near, so nearer handles draw over farther ones.
    std::ranges::sort(handles, {}, &Handle::depth);

    // -----
    // Input
    // -----
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    const bool clicked = ImGui::InvisibleButton(id, ImVec2(radius * 2.0f, radius * 2.0f));
    const bool hovered = ImGui::IsItemHovered();

    // The nearest handle under the cursor.
    const Handle* hoveredHandle = nullptr;
    if (hovered) {
        const ImVec2 mouse = ImGui::GetMousePos();
        for (auto it = handles.rbegin(); it != handles.rend(); ++it) {
            if (DistanceSquared(mouse, it->position) <= handleRadius * handleRadius) {
                hoveredHandle = &*it;
                break;
            }
        }
    }

    // -------
    // Drawing
    // -------
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // Faint, so the handles read on a bright scene; lighter under the cursor.
    drawList->AddCircleFilled(center, radius, hovered ? IM_COL32(255, 255, 255, 40) : IM_COL32(0, 0, 0, 50));

    const float lineWidth = std::max(radius * 0.04f, 1.5f);
    const ImU32 labelColor = IM_COL32(20, 20, 20, 255);
    for (const Handle& handle : handles) {
        const bool highlighted = &handle == hoveredHandle;
        if (handle.positive) {
            drawList->AddLine(center, handle.position, handle.color, lineWidth);
            drawList->AddCircleFilled(handle.position, handleRadius, handle.color);
        } else {
            drawList->AddCircleFilled(handle.position, handleRadius, handle.color);
            drawList->AddCircle(handle.position, handleRadius, AxisColors[handle.axisIndex], 0, lineWidth * 0.75f);
        }
        if (highlighted) drawList->AddCircle(handle.position, handleRadius + lineWidth, IM_COL32_WHITE, 0, lineWidth);

        // Negative ends are named only under the cursor, as in Godot.
        if (handle.positive || highlighted) {
            const ImVec2 textSize = ImGui::CalcTextSize(handle.label);
            const ImVec2 textPosition(handle.position.x - textSize.x * 0.5f, handle.position.y - textSize.y * 0.5f);
            drawList->AddText(textPosition, handle.positive ? labelColor : IM_COL32_WHITE, handle.label);
        }
    }

    if (hoveredHandle) ImGui::SetTooltip("View from %s", hoveredHandle->label);
    if (!clicked || !hoveredHandle) return std::nullopt;

    // Looking already from this side: go round to the other.
    const glm::vec3 toward = -hoveredHandle->axis;
    return glm::dot(camera.GetFront(), toward) > 0.999f ? hoveredHandle->axis : toward;
}

} // namespace GNC
