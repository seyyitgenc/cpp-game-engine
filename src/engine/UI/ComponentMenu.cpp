#include "ComponentMenu.hpp"

#include <string>

#include <imgui.h>

#include "ECS/Component/ComponentRegistry.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Event/EventBus.hpp"
#include "Event/SceneEvents.hpp"

namespace GNC {

void AddComponentMenuItems(Entity& entity, const ComponentRegistry& types, EventBus& bus) {
    for (const ComponentRegistry::Entry& type : types.GetEntries()) {
        const bool present = entity.HasComponent(type.typeID);
        const std::string label = type.icon + " " + type.name;
        if (ImGui::MenuItem(label.c_str(), nullptr, present, !present)) {
            bus.Enqueue<AddComponentEvent>(entity, type.typeID);
        }
    }
}

} // namespace GNC
