#pragma once

#include "ImGuiLayer.hpp"

class ShadersGUI : public ImGuiLayer {
private:
public:
    ShadersGUI();
    ~ShadersGUI();

    void draw() override;
};