#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"

namespace GNC {

class FrustumCulling;
class RenderTarget;

// Geometry pass for deferred rendering
class GeometryPass : public RenderPass {
private:
    FrustumCulling* cullingSystem;

    // G-buffer textures
    RenderTarget* gBuffer;

public:
    GeometryPass(const std::string& name, FrustumCulling* culling);
    ~GeometryPass() override;

protected:
    void BeginPass(vk::raii::CommandBuffer& commandBuffer) override;
    void Render(vk::raii::CommandBuffer& commandBuffer) override;
    void EndPass(vk::raii::CommandBuffer& commandBuffer) override;
};

} // namespace GNC
