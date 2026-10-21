#include "DemoWindowPanel.hpp"

#include <imgui.h>

namespace GNC {

DemoWindowPanel::DemoWindowPanel(const std::string& name) : ImGuiPanel(name) {
    SetVisible(false);
}

void DemoWindowPanel::Render() {
    ImGui::ShowDemoWindow(GetVisibleFlag());
}

} // namespace GNC
