#include "SaveScenePanel.hpp"

#include <algorithm>
#include <filesystem>
#include <string_view>

#include <imgui.h>

#include "Event/EventBus.hpp"
#include "Event/Events.hpp"
#include "Scene/Scene.hpp"
#include "UI/Icons.hpp"
#include "Utils/FileSystem.hpp"

namespace GNC {

namespace {

constexpr std::string_view Extension = ".gncscene";

// What Windows refuses in a file name, plus the separators.
bool IsValidName(std::string_view name) {
    if (name.empty() || name.back() == '.' || name.back() == ' ') return false;
    return std::ranges::none_of(name, [](char c) {
        return std::string_view("<>:\"/\\|?*").find(c) != std::string_view::npos || static_cast<unsigned char>(c) < 32;
    });
}

} // namespace

SaveScenePanel::SaveScenePanel(const std::string& name, EventBus& bus, const Scene& source)
    : ImGuiPanel(name), eventBus(bus), scene(source) {
    SetVisible(false);
}

void SaveScenePanel::Open() {
    const std::string& sceneName = scene.GetName();
    const std::size_t length = sceneName.copy(fileName, sizeof(fileName) - 1);
    fileName[length] = '\0';
    focusName = true;
    SetVisible(true);
}

void SaveScenePanel::Render() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
    if (ImGui::Begin(GetName().c_str(), GetVisibleFlag(), flags)) {
        const std::filesystem::path directory = FileSystem::getSceneDirectory();
        ImGui::TextDisabled("%s", directory.generic_string().c_str());

        if (focusName) {
            ImGui::SetKeyboardFocusHere();
            focusName = false;
        }
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16.0f);
        const bool entered = ImGui::InputText("##FileName", fileName, sizeof(fileName), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        ImGui::TextUnformatted(Extension.data(), Extension.data() + Extension.size());

        const std::string_view name(fileName);
        const bool valid = IsValidName(name);
        const std::filesystem::path path = directory / std::filesystem::path(std::u8string(name.begin(), name.end()) + u8".gncscene");
        std::error_code error;
        if (!valid) {
            ImGui::TextDisabled("Enter a file name");
        } else if (std::filesystem::exists(path, error)) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION " Replaces the existing file");
        } else {
            ImGui::TextUnformatted("");
        }

        ImGui::BeginDisabled(!valid);
        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save") || (entered && valid)) {
            eventBus.Enqueue<SaveSceneEvent>(path);
            SetVisible(false);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) SetVisible(false);
    }
    ImGui::End();
}

} // namespace GNC
