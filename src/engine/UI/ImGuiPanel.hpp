#pragma once

#include <string>

namespace GNC {

// ImGui panel
// One piece of the UI. ImGuiLayer owns the panels and calls Render() on each
// every frame, in the order they were added, between ImGui's NewFrame() and
// Render(). A panel opens its own window (ImGui::Begin(GetName().c_str()) ...
// ImGui::End()), so it picks its own window flags, or draws more than one.
class ImGuiPanel {
private:
    std::string name;

public:
    explicit ImGuiPanel(const std::string& panelName) : name(panelName) {}
    virtual ~ImGuiPanel() = default;

    // Also the title of the panel's window. ImGui identifies windows by their
    // title, so two panels with the same name would share one window, and
    // ImGuiLayer::AddPanel() never adds a second.
    const std::string& GetName() const { return name; }

    virtual void Render() = 0;
};

} // namespace GNC
