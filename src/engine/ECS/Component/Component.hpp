#pragma once

namespace GNC {

// Forward declarations
class Entity;

// Base component class
// Components carry the data and behaviour an entity is made of. The lifecycle
// is a small state machine so setup and teardown happen exactly once, in order,
// no matter who calls Initialize()/Destroy() or how often.
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

    State GetState() const { return state; }
    bool IsActive() const { return state == State::Active; }

    void SetOwner(Entity* entity) { owner = entity; }
    Entity* GetOwner() const { return owner; }
};

} // namespace GNC
