#pragma once

#include <GLFW/glfw3.h>

#include <functional>
#include <unordered_map>
#include <vector>

typedef std::function<void(int)> joy_but_callback;

class Gamepad {
public:
    static void handleEvent(int jid);
    static bool isButtonPushed(int jid, int joysticButton);

    // static bool addCallback(int jid, int button, joy_but_callback);

private:
    // hold GLFW_RELASE, GLFW_PRESS
    static std::array<std::array<int, 32>, 32> _buttons;

    // NOTE: list of joystick buttons callbacks for each joystick
    // std::array<std::array<std::vector<joy_but_callback>, GLFW_GAMEPAD_BUTTON_LAST>, GLFW_JOYSTICK_LAST> _buttonCallbacks;
};
