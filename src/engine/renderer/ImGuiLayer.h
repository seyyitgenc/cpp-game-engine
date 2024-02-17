#pragma once

// note: i am trying this kind of thing first time. there will be tons of error

class ImGuiLayer
{
public:
    ImGuiLayer() = default;
    ~ImGuiLayer() = default;

    void begin();
    void draw();
    void end();
private:
};