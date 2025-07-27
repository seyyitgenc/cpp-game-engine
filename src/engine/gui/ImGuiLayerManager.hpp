#pragma once

#include "ImGuiLayer.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

class ImGuiLayerManager {
public:
    using ImGuiSetupFunc = std::function<void()>;
    static ImGuiLayerManager &instance() {
        static ImGuiLayerManager m_instance;
        return m_instance;
    }

    void addPanel(const std::string &panelName, ImGuiLayer *layer) {
        _guis[panelName].push_back(GuiPtr(layer));
    }

    void draw() {
        if (_firstFrame) {
            if (_setupFunc) {
                _setupFunc();
            } else {
                // use default ImGui setup
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO &io = ImGui::GetIO();
                (void)io;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
                // setup Dear ImGui style
                // ----------------------
                ImGui::StyleColorsDark();

                // setup Platform/Renderer backends
                const char *glsl_version = "#version 460 core";
                ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
                ImGui_ImplOpenGL3_Init(glsl_version);
            }
            _firstFrame = false;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), dockspace_flags);

        if (ImGui::BeginMainMenuBar()) {
            for (size_t i = 0; i < _guis.keys.size(); i++) {
                if (ImGui::BeginMenu(_guis.keys[i].c_str())) {
                    for (size_t j = 0; j < _guis.menus[i].size(); j++) {
                        ImGui::MenuItem(_guis.menus[i][j]->name().c_str(), nullptr, _guis.menus[i][j]->visible());
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMainMenuBar();
        }
        for (size_t i = 0; i < _guis.keys.size(); i++) {
            for (auto &&panel : _guis.menus[i]) {
                if (panel->isVisible()) {
                    panel->draw();
                }
            }
        }
        ImGui::ShowDemoWindow();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void setImGuiSetupFunction(const ImGuiSetupFunc &setupFunc) {
        _setupFunc = setupFunc;
    }

private:
    ImGuiLayerManager(ImGuiLayerManager &) = delete;
    ImGuiLayerManager &operator=(const ImGuiLayerManager &) = delete;

    ImGuiLayerManager() = default;
    ~ImGuiLayerManager() = default;

    ImGuiSetupFunc _setupFunc = nullptr;

private:
    typedef std::unique_ptr<ImGuiLayer> GuiPtr;
    typedef std::vector<GuiPtr> GuiVector;

    struct GUIMap {
        std::vector<std::string> keys;
        std::vector<GuiVector> menus;

        GuiVector &operator[](const std::string key) {
            auto found = std::find(keys.begin(), keys.end(), key);
            if (found == keys.end()) {
                keys.push_back(key);
                menus.push_back(GuiVector());
                return menus.back();
            }
            return menus[std::distance(keys.begin(), found)];
        }
    };

    GUIMap _guis;

    bool _firstFrame = true;
};
