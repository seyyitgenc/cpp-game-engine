#pragma once

#include "Event.hpp"

namespace GNC {

// Input events
// Edge events, enqueued by Window's GLFW callbacks; Window detects double
// clicks and drags itself. Key codes, buttons and modifiers are GLFW's,
// untranslated.

// --------
// Keyboard
// --------

class KeyEvent : public Event {
private:
    int key;
    int scancode;
    int mods;

protected:
    KeyEvent(int keyCode, int scan, int modifiers)
        : key(keyCode), scancode(scan), mods(modifiers) {}

public:
    int GetKey() const { return key; }
    int GetScancode() const { return scancode; }
    int GetMods() const { return mods; }

    // GLFW_MOD_* bit test
    bool HasMod(int mod) const { return (mods & mod) != 0; }
};

class KeyPressedEvent : public KeyEvent {
private:
    bool repeat;

public:
    GNC_EVENT_TYPE(KeyPressedEvent, EventCategory::Input | EventCategory::Keyboard)

    // isRepeat: OS auto-repeat of a held key (GLFW_REPEAT).
    KeyPressedEvent(int keyCode, int scan, int modifiers, bool isRepeat)
        : KeyEvent(keyCode, scan, modifiers), repeat(isRepeat) {}

    bool IsRepeat() const { return repeat; }
};

class KeyReleasedEvent : public KeyEvent {
public:
    GNC_EVENT_TYPE(KeyReleasedEvent, EventCategory::Input | EventCategory::Keyboard)

    KeyReleasedEvent(int keyCode, int scan, int modifiers)
        : KeyEvent(keyCode, scan, modifiers) {}
};

// A Unicode codepoint with layout and modifiers applied: what text input
// wants, not KeyPressedEvent.
class CharTypedEvent : public Event {
private:
    unsigned int codepoint;

public:
    GNC_EVENT_TYPE(CharTypedEvent, EventCategory::Input | EventCategory::Keyboard)

    explicit CharTypedEvent(unsigned int cp) : codepoint(cp) {}

    unsigned int GetCodepoint() const { return codepoint; }
};

// -----
// Mouse
// -----

// Window coordinates. No delta: a listener that needs one keeps the last
// position itself.
class MouseMovedEvent : public Event {
private:
    double x;
    double y;

public:
    GNC_EVENT_TYPE(MouseMovedEvent, EventCategory::Input | EventCategory::Mouse)

    MouseMovedEvent(double posX, double posY) : x(posX), y(posY) {}

    double GetX() const { return x; }
    double GetY() const { return y; }
};

// Motion with a button held. Enqueued after the MouseMovedEvent for the same
// motion, not instead of it.
class MouseDraggedEvent : public Event {
private:
    double x;
    double y;
    int buttons;

public:
    GNC_EVENT_TYPE(MouseDraggedEvent, EventCategory::Input | EventCategory::Mouse)

    // heldButtons: bit (1 << GLFW_MOUSE_BUTTON_*) for each button held.
    MouseDraggedEvent(double posX, double posY, int heldButtons) : x(posX), y(posY), buttons(heldButtons) {}

    double GetX() const { return x; }
    double GetY() const { return y; }
    int GetButtons() const { return buttons; }
    bool IsButtonHeld(int button) const { return (buttons & (1 << button)) != 0; }
};

class MouseScrolledEvent : public Event {
private:
    double offsetX;
    double offsetY;

public:
    GNC_EVENT_TYPE(MouseScrolledEvent, EventCategory::Input | EventCategory::Mouse)

    MouseScrolledEvent(double dx, double dy) : offsetX(dx), offsetY(dy) {}

    double GetOffsetX() const { return offsetX; }
    double GetOffsetY() const { return offsetY; }
};

class MouseButtonEvent : public Event {
private:
    int button;
    int mods;

protected:
    MouseButtonEvent(int mouseButton, int modifiers) : button(mouseButton), mods(modifiers) {}

public:
    int GetButton() const { return button; }
    int GetMods() const { return mods; }
    bool HasMod(int mod) const { return (mods & mod) != 0; }
};

class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    GNC_EVENT_TYPE(MouseButtonPressedEvent,
                   EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton)

    MouseButtonPressedEvent(int mouseButton, int modifiers) : MouseButtonEvent(mouseButton, modifiers) {}
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    GNC_EVENT_TYPE(MouseButtonReleasedEvent,
                   EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton)

    MouseButtonReleasedEvent(int mouseButton, int modifiers) : MouseButtonEvent(mouseButton, modifiers) {}
};

// Enqueued after the second MouseButtonPressedEvent, not instead of it (see
// Window::DoubleClickTime).
class MouseDoubleClickEvent : public MouseButtonEvent {
public:
    GNC_EVENT_TYPE(MouseDoubleClickEvent,
                   EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton)

    MouseDoubleClickEvent(int mouseButton, int modifiers) : MouseButtonEvent(mouseButton, modifiers) {}
};

// The cursor crossed the content area's edge; the ImGui backend needs it.
class MouseEnterEvent : public Event {
public:
    GNC_EVENT_TYPE(MouseEnterEvent, EventCategory::Input | EventCategory::Mouse)
};

class MouseLeaveEvent : public Event {
public:
    GNC_EVENT_TYPE(MouseLeaveEvent, EventCategory::Input | EventCategory::Mouse)
};

// -------
// Gamepad
// -------

// From glfwSetJoystickCallback, which is not installed yet.
class GamepadConnectedEvent : public Event {
private:
    int joystickId;

public:
    GNC_EVENT_TYPE(GamepadConnectedEvent, EventCategory::Input | EventCategory::Gamepad)

    explicit GamepadConnectedEvent(int jid) : joystickId(jid) {}

    int GetJoystickId() const { return joystickId; }
};

class GamepadDisconnectedEvent : public Event {
private:
    int joystickId;

public:
    GNC_EVENT_TYPE(GamepadDisconnectedEvent, EventCategory::Input | EventCategory::Gamepad)

    explicit GamepadDisconnectedEvent(int jid) : joystickId(jid) {}

    int GetJoystickId() const { return joystickId; }
};

} // namespace GNC
