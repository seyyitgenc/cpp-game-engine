#pragma once

#include "ImGuiLayer.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "ImGui/imgui.h"

class ImGuiLayerManager {
public:
    static ImGuiLayerManager &instance() {
        static ImGuiLayerManager m_instance;
        return m_instance;
    }

    void addPanel(const std::string &panelName, ImGuiLayer *layer) {
        _guis[panelName].push_back(GuiPtr(layer));
    }

    void draw() {
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
    }

private:
    ImGuiLayerManager(ImGuiLayerManager &) = delete;
    ImGuiLayerManager &operator=(const ImGuiLayerManager &) = delete;

    ImGuiLayerManager() = default;
    ~ImGuiLayerManager() = default;

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
};
