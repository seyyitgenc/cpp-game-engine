#pragma once

#include <string>

namespace GNC {

// ImGui panel
// One piece of the UI; ImGuiLayer renders the visible ones every frame. A
// panel opens its own window: ImGui::Begin(GetName().c_str(),
// GetVisibleFlag()) gives it a close button that hides it.
class ImGuiPanel {
private:
    std::string name;
    bool visible = true;

public:
    explicit ImGuiPanel(const std::string& panelName) : name(panelName) {}
    virtual ~ImGuiPanel() = default;

    // Also the menu item's label and the window title. ImGui identifies
    // windows by title, so it must be unique.
    const std::string& GetName() const { return name; }

    bool IsVisible() const { return visible; }
    void SetVisible(bool value) { visible = value; }

    // For ImGui's bool* p_open parameters, which clear it on close.
    bool* GetVisibleFlag() { return &visible; }

    // Docks into the central node the first time, before imgui.ini has a
    // place for it. Render()'s first Begin() must be the panel's window.
    virtual bool DocksInCentralNode() const { return false; }

    virtual void Render() = 0;
};

} // namespace GNC
