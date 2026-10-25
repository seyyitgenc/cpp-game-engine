#pragma once

namespace GNC {

class Entity;

// Selection
// The entity picked in the Scene panel and edited in the Inspector. It points
// into the Scene, so clear it before destroying that entity.
class Selection {
private:
    Entity* entity = nullptr;

public:
    Entity* GetEntity() const { return entity; }
    bool IsSelected(const Entity* candidate) const { return candidate && candidate == entity; }

    void Select(Entity* target) { entity = target; }
    void Clear() { entity = nullptr; }
};

} // namespace GNC
