#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Event/EventBus.hpp"
#include "UI/ImGuiPanel.hpp"

struct ImVec2;

namespace GNC {

class Camera;
class Entity;
class Renderer;
class Selection;

// Viewport panel
// The Renderer's scene image, sized to the panel. A left click selects the
// model under it, a second one the entity, and the selection gets a gizmo
// (W/E/R pick move, rotate, scale); other mouse input goes to the camera,
// not ImGui. In a dragged-out OS window it still shows the scene, picks and
// manipulates, but the camera gets no input there.
class ViewportPanel : public ImGuiPanel {
public:
    enum class GizmoTool { Move, Rotate, Scale };

private:
    Renderer& renderer;
    Selection& selection;
    const Camera& camera;

    // As registered with the Vulkan backend, for this scene version.
    vk::DescriptorSet texture;
    std::uint64_t textureVersion = 0;

    GizmoTool gizmoTool = GizmoTool::Move;
    // Scale is always local.
    bool gizmoLocal = false;

    std::vector<EventSubscription> subscriptions;

public:
    // The renderer, selection and camera must outlive the panel.
    ViewportPanel(const std::string& name, Renderer& rend, Selection& selected, const Camera& cam, EventBus& bus);
    // Needs the Vulkan backend, which ImGuiLayer shuts down after its panels.
    ~ViewportPanel() override;

    ViewportPanel(const ViewportPanel&) = delete;
    ViewportPanel& operator=(const ViewportPanel&) = delete;

    bool DocksInCentralNode() const override { return true; }
    void Render() override;

private:
    Entity* ClickTarget(Entity* picked) const;
    void UpdateTexture();
    void RenderGizmo(const ImVec2& min, const ImVec2& size);
    void RenderToolbar(const ImVec2& min);
};

} // namespace GNC
