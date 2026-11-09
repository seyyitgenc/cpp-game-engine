#pragma once

#include "ComponentTypeID.hpp"

namespace GNC {

class Entity;

// Base component class
// The lifecycle is a small state machine, so setup and teardown happen exactly
// once, however often Initialize()/Destroy() are called.
class Component {
public:
    enum class State {
        Uninitialized,
        Initializing,
        Active,
        Destroying,
        Destroyed
    };

private:
    State state = State::Uninitialized;

protected:
    Entity* owner = nullptr;

    // Lifecycle hooks for subclasses
    virtual void OnInitialize() {}
    virtual void OnDestroy() {}

public:
    virtual ~Component() {
        // Last resort: a component that is dropped without an explicit
        // Destroy() still gets to clean up after itself.
        if (state != State::Destroyed) {
            OnDestroy();
            state = State::Destroyed;
        }
    }

    void Initialize() {
        if (state != State::Uninitialized) return;

        state = State::Initializing;
        OnInitialize();
        state = State::Active;
    }

    void Destroy() {
        if (state == State::Destroying || state == State::Destroyed) return;

        state = State::Destroying;
        OnDestroy();
        state = State::Destroyed;
    }

    virtual void Update(float deltaTime) { (void)deltaTime; }
    virtual void Render() {}

    // Filled in by GNC_COMPONENT_TYPE.
    virtual ComponentTypeID GetTypeID() const = 0;
    virtual const char* GetTypeKey() const = 0;
    virtual const char* GetTypeName() const = 0;

    State GetState() const { return state; }
    bool IsActive() const { return state == State::Active; }

    void SetOwner(Entity* entity) { owner = entity; }
    Entity* GetOwner() const { return owner; }
};

} // namespace GNC

// Every concrete component declares its type id, its scene file key and its
// display name with this, as events do with GNC_EVENT_TYPE. The key names
// the type in saved scenes, so it must never change:
//
//     class BoundingVolumeComponent : public Component {
//     public:
//         GNC_COMPONENT_TYPE(BoundingVolumeComponent, "BoundingVolume", "Bounding Volume")
//     };
#define GNC_COMPONENT_TYPE(TypeName, TypeKey, DisplayName)                           \
    static ::GNC::ComponentTypeID StaticTypeID() {                                    \
        return ::GNC::ComponentTypeIDSystem::GetTypeID<TypeName>();                   \
    }                                                                                 \
    static constexpr const char* StaticTypeKey() { return TypeKey; }                  \
    static constexpr const char* StaticTypeName() { return DisplayName; }             \
    ::GNC::ComponentTypeID GetTypeID() const override { return StaticTypeID(); }      \
    const char* GetTypeKey() const override { return StaticTypeKey(); }               \
    const char* GetTypeName() const override { return StaticTypeName(); }
