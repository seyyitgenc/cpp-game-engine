#include "gamepad.hpp"

#include "util/log.hpp"
std::array<std::array<int, 32>, 32> Gamepad::_buttons;

void Gamepad::handleEvent(int jid)
{
    int axesCount = 0;
    int buttonCount = 0;
    const float* axesReturnVal = glfwGetJoystickAxes(jid, &axesCount);
    const unsigned char* buttonReturnVal = glfwGetJoystickButtons(jid, &buttonCount);

    if (!axesReturnVal || !buttonReturnVal) {
        Log::writeFormatted(Log::Fatal, "WTF RETURNVAL IS NULL");
        _buttons[jid] = { 0 };
        return;
    }

    // for (int i = 0; i < axesCount; i++) {
    //     Log::writeFormatted(Log::Info, "axes: %d value: %0.2f", i, axesReturnVal[i]);
    // }

    for (int i = 0; i < buttonCount; i++) {
        _buttons[jid][i] = buttonReturnVal[i];
        if (_buttons[jid][i] == GLFW_PRESS) {
            std::cout << "clicked " << i << std::endl;
        }
    }
}

bool Gamepad::isButtonPushed(int jid, int joysticButton)
{
    return _buttons[jid][joysticButton] == GLFW_PRESS;
}

// bool Gamepad::addCallback(int jid, int button, joy_but_callback)
// {
// }
