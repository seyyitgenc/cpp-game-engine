#include "InspectorPanel.hpp"

#include <cfloat>
#include <memory>
#include <string>

#include <imgui.h>

#include "ECS/Component/ComponentRegistry.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Event/EventBus.hpp"
#include "Event/SceneEvents.hpp"
#include "UI/ComponentMenu.hpp"
#include "UI/Icons.hpp"
#include "UI/PropertyEditor.hpp"
#include "UI/Selection.hpp"

namespace GNC {

namespace {

constexpr const char* UnregisteredIcon = ICON_FA_PUZZLE_PIECE;

constexpr std::size_t MaxNameLength = 127;

// At 100% scale.
constexpr float InitialWidth = 340.0f;
constexpr float InitialHeight = 460.0f;

// Label room, in font heights. ImGui's default share of the width cuts
// labels off in a narrow window.
constexpr float LabelColumnWidth = 7.0f;

} // namespace

InspectorPanel::InspectorPanel(const std::string& name, const Selection& selected, const ComponentRegistry& types, ResourceManager& res, EventBus& bus)
    : ImGuiPanel(name), selection(selected), componentTypes(types), resources(res), eventBus(bus) {}

void InspectorPanel::Render() {
    // Left to fit itself, a new window would shrink to the hint text below.
    const float scale = ImGui::GetStyle().FontScaleDpi;
    ImGui::SetNextWindowSize(ImVec2(InitialWidth * scale, InitialHeight * scale), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(GetName().c_str(), GetVisibleFlag())) {
        if (Entity* entity = selection.GetEntity()) {
            RenderEntity(*entity);
        } else {
            ImGui::TextDisabled("Select an entity in the Scene panel or the Viewport.");
        }
    }
    ImGui::End();
}

void InspectorPanel::RenderEntity(Entity& entity) {
    // So widget state (a half-typed name) does not carry over to the next
    // entity.
    ImGui::PushID(&entity);

    // ------
    // Header
    // ------
    bool active = entity.IsActive();
    if (ImGui::Checkbox("##Active", &active)) entity.SetActive(active);
    ImGui::SetItemTooltip("Active: updated and drawn");
    ImGui::SameLine();

    char name[MaxNameLength + 1];
    const std::size_t length = entity.GetName().copy(name, MaxNameLength);
    name[length] = '\0';
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##Name", name, sizeof(name))) entity.SetName(name);

    // ----------
    // Components
    // ----------
    for (const std::unique_ptr<Component>& component : entity.GetComponents()) {
        RenderComponent(entity, *component);
    }

    // -------------
    // Add Component
    // -------------
    ImGui::Spacing();
    if (ImGui::Button(ICON_FA_CIRCLE_PLUS " Add Component", ImVec2(-FLT_MIN, 0.0f))) ImGui::OpenPopup("##AddComponent");
    if (ImGui::BeginPopup("##AddComponent")) {
        AddComponentMenuItems(entity, componentTypes, eventBus);
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

void InspectorPanel::RenderComponent(Entity& entity, Component& component) {
    const ComponentRegistry::Entry* type = componentTypes.Find(component.GetTypeID());
    const std::string label = std::string(type ? type->icon.c_str() : UnregisteredIcon) + " " + component.GetTypeName();

    ImGui::PushID(&component);
    // AllowOverlap: the remove button sits on top of the header.
    const bool open = ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);

    // Right-aligned on the header. A small button has no vertical padding,
    // hence the + FramePadding.y.
    const ImVec2 headerMin = ImGui::GetItemRectMin();
    const ImVec2 headerMax = ImGui::GetItemRectMax();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float buttonWidth = ImGui::CalcTextSize(ICON_FA_TRASH_CAN).x + style.FramePadding.x * 2.0f;
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(headerMax.x - buttonWidth - style.FramePadding.x, headerMin.y + style.FramePadding.y));
    if (ImGui::SmallButton(ICON_FA_TRASH_CAN)) eventBus.Enqueue<RemoveComponentEvent>(entity, component.GetTypeID());
    ImGui::SetItemTooltip("Remove Component");

    if (open) {
        if (type && !type->properties.empty()) {
            ImGui::PushItemWidth(-ImGui::GetFontSize() * LabelColumnWidth);
            EditProperties(component, type->properties, resources);
            ImGui::PopItemWidth();
        } else {
            ImGui::TextDisabled("Nothing to edit");
        }
    }
    ImGui::PopID();
}

} // namespace GNC
