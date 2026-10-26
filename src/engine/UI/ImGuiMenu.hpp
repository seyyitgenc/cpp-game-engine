#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace GNC {

class ImGuiPanel;

struct MenuItemOptions {
    // Display only; bind the key with an event listener.
    std::string shortcut {};

    std::function<bool()> enabled {};

    // A command shows a check mark while this returns true.
    std::function<bool()> checked {};
};

// ImGui menu
// A menu of the main menu bar, or the bar itself (ImGuiLayer::GetMenuBar()).
// Commands run inside the ImGui frame: to change anything outside the UI,
// enqueue an event (as File > Quit does) rather than acting directly.
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

    // In drawing order.
    std::vector<Entry> entries;

public:
    // Found, or added. The reference stays valid as entries are added.
    ImGuiMenu& GetMenu(const std::string& label);

    void AddItem(const std::string& label, std::function<void()> action, MenuItemOptions options = {});

    // A check mark item that shows and hides the panel, which must outlive
    // the menu.
    void AddPanelItem(ImGuiPanel& panel, MenuItemOptions options = {});

    void AddSeparator();

    // Into the current menu, or menu bar.
    void Render() const;
};

} // namespace GNC
