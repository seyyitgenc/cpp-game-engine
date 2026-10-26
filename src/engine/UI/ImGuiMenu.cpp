#include "ImGuiMenu.hpp"

#include <algorithm>
#include <utility>

#include <imgui.h>

#include "ImGuiPanel.hpp"

namespace GNC {

ImGuiMenu& ImGuiMenu::GetMenu(const std::string& label) {
    const auto it = std::ranges::find_if(entries, [&](const Entry& entry) {
        return entry.submenu && entry.label == label;
    });
    if (it != entries.end()) return *it->submenu;

    entries.push_back({ .label = label, .submenu = std::make_unique<ImGuiMenu>() });
    return *entries.back().submenu;
}

void ImGuiMenu::AddItem(const std::string& label, std::function<void()> action, MenuItemOptions options) {
    entries.push_back({ .label = label, .action = std::move(action), .options = std::move(options) });
}

void ImGuiMenu::AddPanelItem(ImGuiPanel& panel, MenuItemOptions options) {
    entries.push_back({ .label = panel.GetName(), .panel = &panel, .options = std::move(options) });
}

void ImGuiMenu::AddSeparator() {
    entries.emplace_back();
}

void ImGuiMenu::Render() const {
    for (const Entry& entry : entries) {
        if (entry.submenu) {
            if (ImGui::BeginMenu(entry.label.c_str())) {
                entry.submenu->Render();
                ImGui::EndMenu();
            }
            continue;
        }

        const char* shortcut = entry.options.shortcut.empty() ? nullptr : entry.options.shortcut.c_str();
        const bool enabled = !entry.options.enabled || entry.options.enabled();

        if (entry.panel) {
            // Toggles the panel's flag by itself when clicked.
            ImGui::MenuItem(entry.label.c_str(), shortcut, entry.panel->GetVisibleFlag(), enabled);
        } else if (entry.action) {
            const bool checked = entry.options.checked && entry.options.checked();
            if (ImGui::MenuItem(entry.label.c_str(), shortcut, checked, enabled)) entry.action();
        } else {
            ImGui::Separator();
        }
    }
}

} // namespace GNC
