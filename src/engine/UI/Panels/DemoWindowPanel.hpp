#pragma once

#include <string>

#include "UI/ImGuiPanel.hpp"

namespace GNC {

// Demo window panel
class DemoWindowPanel : public ImGuiPanel {
public:
    explicit DemoWindowPanel(const std::string& name);

    void Render() override;
};

} // namespace GNC
