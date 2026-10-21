#pragma once

#include <string>

namespace GNC {

// ImGui panel
// One piece of the UI. ImGuiLayer owns the panels and calls Render() on each
// visible one every frame, in the order they were added, between ImGui's
// NewFrame() and Render(). A panel opens its own window (ImGui::Begin(
// GetName().c_str(), GetVisibleFlag()) ... ImGui::End()), so it picks its own
// window flags, or draws more than one.
//
// Passing GetVisibleFlag() to Begin() gives the window a close button, which
// hides the panel; an ImGuiMenu::AddPanelItem() in the menu bar shows it
// again.
class ImGuiPanel {
private:
    std::string name;
    bool visible = true;

public:
    explicit ImGuiPanel(const std::string& panelName) : name(panelName) {}
    virtual ~ImGuiPanel() = default;

    // Also the label of the panel's menu item and, by convention, the title
    // of its window. ImGui identifies windows by their title, so two panels
    // with the same name would share one window, and ImGuiLayer::AddPanel()
    // never adds a second.
    const std::string& GetName() const { return name; }

    // A hidden panel is not rendered.
    bool IsVisible() const { return visible; }
    void SetVisible(bool value) { visible = value; }

    // For the ImGui calls that take a bool* p_open (Begin, MenuItem,
    // ShowDemoWindow), which clear it when the window is closed.
    bool* GetVisibleFlag() { return &visible; }

    // True to dock into the dockspace's central node the first time the
    // window opens, before imgui.ini has a place for it. Render()'s first
    // Begin() must then be the panel's own window.
    virtual bool DocksInCentralNode() const { return false; }

    // Called only while the panel is visible.
    virtual void Render() = 0;
};

} // namespace GNC
