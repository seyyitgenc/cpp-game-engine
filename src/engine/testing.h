#pragma once


#include "globals.h"
#include "model.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "gui/gui.h"
#include <vector>
#include "renderer/framebuffer.h"


class Scene
{
public:
    Scene() = default;
    ~Scene() = default;
    void draw(){
        for(auto &&i : listOfObjects)
            i.first.Draw(i.second);
    };
private:
    std::vector<std::pair<Model, Shader>> listOfObjects;
    // std::vector<Decal> listOfObjects;
    // Camera* activeCam = nullptr; // default camera pointer
    FrameBuffer gBuffer;
};

struct ImGuiLayer
{
    void beginFrame(){
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    void endFrame(){
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

class Renderer{
    void draw(){
        // draw imgui components
        imguiLayer.beginFrame();

        Gui::Init();
        ImGui::ShowDemoWindow();

        imguiLayer.endFrame();
    }
private:
    ImGuiLayer imguiLayer;
};