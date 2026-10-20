#pragma once

#include <string>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

class Camera;

// Engine panel
// Frame rate and camera position, plus the switch for ImGui's demo window,
// which shows every widget ImGui has along with the code that draws it.
class EnginePanel : public ImGuiPanel {
private:
    const Camera& camera;
    bool showDemoWindow = false;

public:
    // The camera must outlive the panel.
    EnginePanel(const std::string& name, const Camera& cam);

    void Render() override;
};

} // namespace GNC
