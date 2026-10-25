#pragma once

#include <string>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

class Component;
class ComponentRegistry;
class Entity;
class EventBus;
class Scene;
class Selection;

// Scene panel
// The entity hierarchy as a tree, each entity opening onto its components
// and children, inactive ones greyed out. Selects for the Inspector;
// creates entities and adds and removes components through
// Event/SceneEvents.hpp.
class ScenePanel : public ImGuiPanel {
private:
    const Scene& scene;
    const ComponentRegistry& componentTypes;
    Selection& selection;
    EventBus& eventBus;

    // Compared, never dereferenced: it may have been destroyed since.
    const Entity* shownSelection = nullptr;
    bool revealSelection = false;

public:
    // The scene, registry, selection and bus must outlive the panel.
    ScenePanel(const std::string& name, const Scene& scn, const ComponentRegistry& types, Selection& selected, EventBus& bus);

    void Render() override;

private:
    void RenderEntity(Entity& entity);
    void RenderEntityMenu(Entity& entity);
    void RenderComponentMenu(Entity& entity, const Component& component);
    void SelectOnClick(Entity& entity);
};

} // namespace GNC
