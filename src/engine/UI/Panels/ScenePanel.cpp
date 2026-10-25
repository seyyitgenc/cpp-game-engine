#include "ScenePanel.hpp"

#include <memory>

#include <imgui.h>

#include "ECS/Component/ComponentRegistry.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Event/EventBus.hpp"
#include "Event/SceneEvents.hpp"
#include "Scene/Scene.hpp"
#include "UI/ComponentMenu.hpp"
#include "UI/Icons.hpp"
#include "UI/Selection.hpp"

namespace GNC {

namespace {

constexpr const char* UnregisteredIcon = ICON_FA_PUZZLE_PIECE;

constexpr const char* NewEntityName = "Entity";

// Each child draws its own line to its parent, so every node needs the
// flag, not only the ones with children.
constexpr ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;
constexpr ImGuiTreeNodeFlags ComponentFlags = NodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

// By address rather than label: names need not be unique.
bool TreeNode(const void* id, ImGuiTreeNodeFlags flags, const char* icon, const char* name, bool active) {
    if (!active) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    const bool open = ImGui::TreeNodeEx(id, flags, "%s %s", icon, name);
    if (!active) ImGui::PopStyleColor();
    return open;
}

} // namespace

ScenePanel::ScenePanel(const std::string& name, const Scene& scn, const ComponentRegistry& types, Selection& selected, EventBus& bus)
    : ImGuiPanel(name), scene(scn), componentTypes(types), selection(selected), eventBus(bus) {}

void ScenePanel::Render() {
    if (ImGui::Begin(GetName().c_str(), GetVisibleFlag())) {
        if (ImGui::Button(ICON_FA_CIRCLE_PLUS " Create Entity")) eventBus.Enqueue<CreateEntityEvent>(NewEntityName);
        ImGui::Separator();

        // A child window, so the button stays put while the list scrolls.
        if (ImGui::BeginChild("##Entities")) {
            for (Entity* entity : scene.GetEntities()) {
                RenderEntity(*entity);
            }

            if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem(ICON_FA_CIRCLE_PLUS " Create Entity")) eventBus.Enqueue<CreateEntityEvent>(NewEntityName);
                ImGui::EndPopup();
            }

            // IsWindowHovered() is false while a popup is open, so the click
            // that closes one does not clear the selection.
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
                selection.Clear();
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void ScenePanel::RenderEntity(Entity& entity) {
    const auto& components = entity.GetComponents();

    // Clicking the name selects; the arrow, or a double click, opens.
    ImGuiTreeNodeFlags flags = NodeFlags | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (components.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
    if (selection.IsSelected(&entity)) flags |= ImGuiTreeNodeFlags_Selected;

    const bool open = TreeNode(&entity, flags, ICON_FA_CUBE, entity.GetName().c_str(), entity.IsActive());
    SelectOnClick(entity);
    if (ImGui::BeginPopupContextItem()) {
        RenderEntityMenu(entity);
        ImGui::EndPopup();
    }
    if (!open) return;

    for (const std::unique_ptr<Component>& component : components) {
        const ComponentRegistry::Entry* type = componentTypes.Find(component->GetTypeID());
        const char* icon = type ? type->icon.c_str() : UnregisteredIcon;
        TreeNode(component.get(), ComponentFlags, icon, component->GetTypeName(), entity.IsActive() && component->IsActive());
        // A component row stands for its entity.
        SelectOnClick(entity);
        if (ImGui::BeginPopupContextItem()) {
            RenderComponentMenu(entity, *component);
            ImGui::EndPopup();
        }
    }
    ImGui::TreePop();
}

void ScenePanel::RenderEntityMenu(Entity& entity) {
    if (!ImGui::BeginMenu(ICON_FA_CIRCLE_PLUS " Add Component")) return;

    AddComponentMenuItems(entity, componentTypes, eventBus);
    ImGui::EndMenu();
}

void ScenePanel::RenderComponentMenu(Entity& entity, const Component& component) {
    if (ImGui::MenuItem(ICON_FA_TRASH_CAN " Remove Component")) {
        eventBus.Enqueue<RemoveComponentEvent>(entity, component.GetTypeID());
    }
}

// Right clicks select too, so a row's menu acts on the entity the Inspector
// shows.
void ScenePanel::SelectOnClick(Entity& entity) {
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
    if (clicked && !ImGui::IsItemToggledOpen()) selection.Select(&entity);
}

} // namespace GNC
