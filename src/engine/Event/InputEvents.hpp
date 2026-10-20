#pragma once

#include "Event.hpp"

namespace GNC {

// Input events
// Edge events only: a key going down or up, a button changing, the cursor
// moving. The GLFW callbacks in Window.cpp enqueue these. Double clicks and
// drags are not GLFW events; Window detects them.
//
// "Is W currently held" is state. Keep it from KeyPressed/KeyReleased (see
// CameraController) rather than polling Window::IsKeyDown(): polling skips
// the bus, so a higher priority listener (a UI layer) cannot swallow the key
// first. Never return true for a release, or a listener below you keeps the
// key held forever.
//
// Key codes, button ids and modifier bits are GLFW's, passed through
// untranslated. The engine has one windowing backend and an abstraction layer
// over its constants would be a second copy of the same table.

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

    // repeat is true for GLFW_REPEAT: the key was already down and the OS
    // auto-repeat fired. Text input wants those; a "toggle wireframe"
    // binding does not.
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

// Unicode codepoint from the character callback; already has layout and
// modifiers applied. This is what a text field wants, not KeyPressedEvent.
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

// Cursor position in window coordinates. Deltas are not carried here: a
// listener that wants them keeps the last position itself, which is also
// where "first mouse" handling belongs.
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

// Cursor motion while at least one mouse button is held. Enqueued right after
// the MouseMovedEvent for the same motion, not instead of it, so a listener
// that wants every cursor position (hover, a UI layer) needs only
// MouseMovedEvent.
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

// A second press of the same button, quickly and close to the first (see
// Window::DoubleClickTime). Enqueued right after that second
// MouseButtonPressedEvent, not instead of it, so a listener tracking which
// buttons are down never misses a press. A third press starts over.
class MouseDoubleClickEvent : public MouseButtonEvent {
public:
    GNC_EVENT_TYPE(MouseDoubleClickEvent,
                   EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton)

    MouseDoubleClickEvent(int mouseButton, int modifiers) : MouseButtonEvent(mouseButton, modifiers) {}
};

// -------
// Gamepad
// -------

// From glfwSetJoystickCallback (not installed yet: there is no gamepad
// support). Button and axis state would be polled; connection changes are the
// only real edges GLFW reports for controllers.
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
