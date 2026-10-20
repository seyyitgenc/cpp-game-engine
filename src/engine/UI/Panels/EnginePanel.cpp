#include "EnginePanel.hpp"

#include <glm/glm.hpp>
#include <imgui.h>

#include "Camera/Camera.hpp"

namespace GNC {

EnginePanel::EnginePanel(const std::string& name, const Camera& cam) : ImGuiPanel(name), camera(cam) {}

void EnginePanel::Render() {
    if (ImGui::Begin(GetName().c_str())) {
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);

        const glm::vec3& position = camera.GetPosition();
        ImGui::Text("Camera: %.2f, %.2f, %.2f", position.x, position.y, position.z);

        ImGui::Checkbox("ImGui demo window", &showDemoWindow);
    }
    ImGui::End();

    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}

} // namespace GNC
