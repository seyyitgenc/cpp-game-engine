#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace GNC {

class ImGuiPanel;

// The optional parts of a menu item.
struct MenuItemOptions {
    // Shown next to the label, for display only.
    std::string shortcut {};

    std::function<bool()> enabled {};
};

// ImGui menu
// A menu of the main menu bar, or the bar itself (ImGuiLayer::GetMenuBar()).
// Holds, in the order added: submenus, commands, panel toggles and
// separators.
//
// Commands run inside the ImGui frame, from ImGuiLayer::Render(), before the
// frame is drawn. To change anything outside the UI, enqueue an event (as
// File > Quit enqueues QuitApplicationEvent, or a GNC_EVENT_TYPE user event
// for a save or a load) rather than acting directly.
class ImGuiMenu {
private:
    // Exactly one of submenu, panel and action, or none: a separator.
    struct Entry {
        std::string label {};
        std::unique_ptr<ImGuiMenu> submenu {};
        ImGuiPanel* panel = nullptr;
        std::function<void()> action {};
        MenuItemOptions options {};
    };

    // In the order added, which is the order they are drawn in.
    std::vector<Entry> entries;

public:
    // The submenu with this label, added after the entries so far when there
    // is none yet. Submenus live behind unique_ptr, so the reference stays
    // valid while more entries are added.
    ImGuiMenu& GetMenu(const std::string& label);

    // A command: runs action when clicked.
    void AddItem(const std::string& label, std::function<void()> action, MenuItemOptions options = {});

    // A check mark item labelled with the panel's name, which shows and hides
    // it. The panel must outlive the menu; ImGuiLayer owns both.
    void AddPanelItem(ImGuiPanel& panel, MenuItemOptions options = {});

    void AddSeparator();

    // Draws the entries into the current menu, or menu bar. ImGuiLayer calls
    // it on the bar between BeginMainMenuBar() and EndMainMenuBar().
    void Render() const;
};

} // namespace GNC
