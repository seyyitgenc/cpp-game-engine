#pragma once

#include <string>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

class Component;
class ComponentRegistry;
class Entity;
class EventBus;
class ResourceManager;
class Selection;

// Inspector panel
// The selected entity's active flag, name and component properties. Values
// are edited in place; adding and removing components goes through
// Event/SceneEvents.hpp.
class InspectorPanel : public ImGuiPanel {
private:
    const Selection& selection;
    const ComponentRegistry& componentTypes;
    ResourceManager& resources;
    EventBus& eventBus;

public:
    // The selection, registry, resources and bus must outlive the panel.
    InspectorPanel(const std::string& name, const Selection& selected, const ComponentRegistry& types, ResourceManager& res, EventBus& bus);

    void Render() override;

private:
    void RenderEntity(Entity& entity);
    void RenderComponent(Entity& entity, Component& component);
};

} // namespace GNC
