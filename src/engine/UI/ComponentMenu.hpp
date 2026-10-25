#pragma once

namespace GNC {

class ComponentRegistry;
class Entity;
class EventBus;

// Add Component items, for inside a BeginMenu() or BeginPopup(). Types the
// entity already has are checked and disabled; picking one enqueues an
// AddComponentEvent.
void AddComponentMenuItems(Entity& entity, const ComponentRegistry& types, EventBus& bus);

} // namespace GNC
