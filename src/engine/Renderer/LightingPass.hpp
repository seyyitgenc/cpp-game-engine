#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"

namespace GNC {

class GeometryPass;
class Light;

// Lighting pass for deferred rendering
class LightingPass : public RenderPass {
private:
    GeometryPass* geometryPass;
    std::vector<Light*> lights;

public:
    LightingPass(const std::string& name, GeometryPass* gPass);

    void AddLight(Light* light);
    void RemoveLight(Light* light);

protected:
    void BeginPass(const FrameContext& frame) override;
    void Render(const FrameContext& frame) override;
    void EndPass(const FrameContext& frame) override;
};

} // namespace GNC
