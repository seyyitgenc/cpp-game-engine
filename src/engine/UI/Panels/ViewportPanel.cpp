#include "ViewportPanel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#include "Camera/Camera.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Event/Events.hpp"
#include "Renderer/Renderer.hpp"
#include "UI/Icons.hpp"
#include "UI/Selection.hpp"

namespace GNC {

namespace {

ImGuizmo::OPERATION ToOperation(ViewportPanel::GizmoTool tool) {
    switch (tool) {
        case ViewportPanel::GizmoTool::Move: return ImGuizmo::TRANSLATE;
        case ViewportPanel::GizmoTool::Rotate: return ImGuizmo::ROTATE;
        case ViewportPanel::GizmoTool::Scale: return ImGuizmo::SCALE;
    }
    return ImGuizmo::TRANSLATE;
}

} // namespace

ViewportPanel::ViewportPanel(const std::string& name, Renderer& rend, Selection& selected, const Camera& cam, EventBus& bus)
    : ImGuiPanel(name), renderer(rend), selection(selected), camera(cam) {
    // Below the camera, which keeps W and E while it looks.
    subscriptions.push_back(bus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (e.IsRepeat() || e.GetMods() != 0) return false;

        switch (e.GetKey()) {
            case GLFW_KEY_W: gizmoTool = GizmoTool::Move; return true;
            case GLFW_KEY_E: gizmoTool = GizmoTool::Rotate; return true;
            case GLFW_KEY_R: gizmoTool = GizmoTool::Scale; return true;
            default: return false;
        }
    }));
}

ViewportPanel::~ViewportPanel() {
    if (texture) ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
}

void ViewportPanel::Render() {
    // A click on the background answers nullptr, which deselects.
    if (const std::optional<Entity*> picked = renderer.TakePickResult()) selection.Select(*picked);

    // ImGuizmo only takes a press while no ImGui item is hovered or active,
    // so over the gizmo (as of last frame) the image gets no button, and the
    // window must not move in its place.
    const bool overGizmo = ImGuizmo::IsOver();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (overGizmo) flags |= ImGuiWindowFlags_NoMove;

    // No padding: the image goes edge to edge.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool open = ImGui::Begin(GetName().c_str(), GetVisibleFlag(), flags);
    ImGui::PopStyleVar();

    std::optional<vk::Offset2D> hoverPixel;
    if (open) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const ImVec2 size(std::max(available.x, 1.0f), std::max(available.y, 1.0f));
        const ImVec2 scale = ImGui::GetWindowViewport()->FramebufferScale;

        // May rebuild the scene image, so before UpdateTexture().
        renderer.SetSceneExtent({
            static_cast<std::uint32_t>(size.x * scale.x),
            static_cast<std::uint32_t>(size.y * scale.y)
        });
        UpdateTexture();

        const ImVec2 min = ImGui::GetCursorScreenPos();
        auto toScenePixel = [&](ImVec2 position) {
            return vk::Offset2D {
                .x = static_cast<std::int32_t>((position.x - min.x) * scale.x),
                .y = static_cast<std::int32_t>((position.y - min.y) * scale.y)
            };
        };

        // The button takes left clicks, so dragging over the image does not
        // move a floating viewport. The right button is left to the camera.
        // A drag is not a click (the delta stays zero until it passes the
        // drag threshold), and the pick goes where the press was. The
        // toolbar overlaps it.
        bool imageHovered = false;
        if (!overGizmo) {
            const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            ImGui::SetNextItemAllowOverlap();
            if (ImGui::InvisibleButton("##scene", size) && drag.x == 0.0f && drag.y == 0.0f) {
                renderer.RequestPick(toScenePixel(ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Left]));
            }
            imageHovered = ImGui::IsItemHovered();
        }
        const ImTextureID textureId = reinterpret_cast<std::uintptr_t>(static_cast<VkDescriptorSet>(texture));
        ImGui::GetWindowDrawList()->AddImage(textureId, min, ImVec2(min.x + size.x, min.y + size.y));

        // Not hovered while the camera looks: the cursor is captured.
        if (imageHovered) hoverPixel = toScenePixel(ImGui::GetMousePos());
        // Otherwise ImGui would want the mouse and swallow it, and the right
        // button would not reach the camera.
        if (imageHovered || (overGizmo && ImGui::IsWindowHovered())) ImGui::SetNextFrameWantCaptureMouse(false);

        RenderGizmo(min, size);
        RenderToolbar(min);
    }
    ImGui::End();

    renderer.SetHoverPixel(hoverPixel);
    renderer.SetOutlinedEntities(selection.GetEntity(), renderer.GetHoveredEntity());
}

// Every new scene version follows a wait for the GPU, so nothing in flight
// still reads the old descriptor set.
void ViewportPanel::UpdateTexture() {
    if (texture && textureVersion == renderer.GetSceneVersion()) return;

    if (texture) ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
    texture = ImGui_ImplVulkan_AddTexture(static_cast<VkImageView>(renderer.GetSceneView()), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    textureVersion = renderer.GetSceneVersion();
}

void ViewportPanel::RenderGizmo(const ImVec2& min, const ImVec2& size) {
    Entity* entity = selection.GetEntity();
    TransformComponent* transform = entity ? entity->GetComponent<TransformComponent>() : nullptr;
    if (!transform) {
        // ImGuizmo ends a drag only inside Manipulate(), so one whose entity
        // was deleted mid-drag would keep IsOver() true for good.
        if (ImGuizmo::IsUsingAny()) {
            ImGuizmo::Enable(false);
            ImGuizmo::Enable(true);
        }
        return;
    }

    // ImGuizmo projects into OpenGL's clip space, Y up, so Camera's Vulkan
    // flip is undone. Its 0..1 depth is fine.
    glm::mat4 projection = camera.GetProjectionMatrix();
    projection[1][1] *= -1.0f;

    ImGuizmo::SetOrthographic(camera.GetProjectionType() == ProjectionType::Orthographic);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(min.x, min.y, size.x, size.y);

    glm::mat4 model = transform->GetTransformMatrix();
    if (!ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()), glm::value_ptr(projection), ToOperation(gizmoTool),
            gizmoLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD, glm::value_ptr(model))) {
        return;
    }

    // Only what the tool changes, so rounding does not creep into the rest.
    switch (gizmoTool) {
        case GizmoTool::Move:
            transform->SetPosition(glm::vec3(model[3]));
            break;
        case GizmoTool::Rotate: {
            // The columns are the axes times the scale, which did not change.
            const glm::vec3 scale = transform->GetScale();
            if (glm::any(glm::equal(scale, glm::vec3(0.0f)))) break;
            const glm::mat3 axes { glm::vec3(model[0]) / scale.x, glm::vec3(model[1]) / scale.y, glm::vec3(model[2]) / scale.z };
            transform->SetRotation(glm::normalize(glm::quat_cast(axes)));
            break;
        }
        case GizmoTool::Scale: {
            // Lengths drop the sign of a mirrored axis.
            const glm::vec3 previous = transform->GetScale();
            transform->SetScale({
                std::copysign(glm::length(glm::vec3(model[0])), previous.x),
                std::copysign(glm::length(glm::vec3(model[1])), previous.y),
                std::copysign(glm::length(glm::vec3(model[2])), previous.z)
            });
            break;
        }
    }
}

void ViewportPanel::RenderToolbar(const ImVec2& min) {
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetCursorScreenPos(ImVec2(min.x + style.ItemSpacing.x, min.y + style.ItemSpacing.y));

    struct Tool {
        GizmoTool tool;
        const char* icon;
        const char* tooltip;
    };
    static constexpr Tool Tools[] = {
        { GizmoTool::Move, ICON_FA_UP_DOWN_LEFT_RIGHT, "Move (W)" },
        { GizmoTool::Rotate, ICON_FA_ROTATE, "Rotate (E)" },
        { GizmoTool::Scale, ICON_FA_EXPAND, "Scale (R)" },
    };

    for (const Tool& tool : Tools) {
        const bool active = gizmoTool == tool.tool;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
        if (ImGui::Button(tool.icon)) gizmoTool = tool.tool;
        if (active) ImGui::PopStyleColor();
        ImGui::SetItemTooltip("%s", tool.tooltip);
        ImGui::SameLine();
    }

    // ImGuizmo always scales along the local axes.
    ImGui::BeginDisabled(gizmoTool == GizmoTool::Scale);
    if (ImGui::Button(gizmoLocal ? ICON_FA_CUBE " Local###space" : ICON_FA_GLOBE " Global###space")) gizmoLocal = !gizmoLocal;
    ImGui::EndDisabled();
}

} // namespace GNC
