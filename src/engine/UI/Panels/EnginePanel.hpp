#pragma once

#include <string>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

class Camera;

// Engine panel
// Frame rate and camera position.
class EnginePanel : public ImGuiPanel {
private:
    const Camera& camera;

public:
    // The camera must outlive the panel.
    EnginePanel(const std::string& name, const Camera& cam);

    void Render() override;
};

} // namespace GNC
