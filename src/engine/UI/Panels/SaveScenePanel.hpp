#pragma once

#include <string>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

class EventBus;
class Scene;

// Save Scene As
// A small window asking for a file name under the scene directory
// (FileSystem::getSceneDirectory()); Save enqueues a SaveSceneEvent. Hidden
// until Open().
class SaveScenePanel : public ImGuiPanel {
private:
    EventBus& eventBus;
    const Scene& scene;

    char fileName[128] {};
    // Focus the name field the frame the window appears.
    bool focusName = false;

public:
    // The bus and the scene must outlive the panel.
    SaveScenePanel(const std::string& name, EventBus& bus, const Scene& source);

    // Shows the window, the name field filled with the scene's name.
    void Open();

    void Render() override;
};

} // namespace GNC
