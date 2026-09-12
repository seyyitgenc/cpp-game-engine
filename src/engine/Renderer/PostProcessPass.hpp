#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace GNC {

class LightingPass;

// Post-process effect base class
class PostProcessEffect {
public:
    virtual ~PostProcessEffect() = default;
    virtual void Apply(vk::raii::CommandBuffer& commandBuffer) = 0;
};

// Post-processing pass
class PostProcessPass : public RenderPass {
private:
    LightingPass* lightingPass;
    std::vector<PostProcessEffect*> effects;

public:
    PostProcessPass(const std::string& name, LightingPass* lPass);

    void AddEffect(PostProcessEffect* effect);
    void RemoveEffect(PostProcessEffect* effect);

protected:
    void BeginPass(vk::raii::CommandBuffer& commandBuffer) override  {
        // Begin rendering with dynamic rendering
        vk::RenderingInfoKHR renderingInfo;

        // Set up color attachment for the post-processing pass
        vk::RenderingAttachmentInfoKHR colorAttachment;
        colorAttachment.setImageView(GetRenderTarget()->GetColorImageView())
                       .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                       .setLoadOp(vk::AttachmentLoadOp::eClear)
                       .setStoreOp(vk::AttachmentStoreOp::eStore)
                       .setClearValue(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));

        // Configure rendering info
        renderingInfo.setRenderArea(vk::Rect2D({0, 0}, {GetRenderTarget()->GetWidth(), GetRenderTarget()->GetHeight()}))
                     .setLayerCount(1)
                     .setColorAttachmentCount(1)
                     .setPColorAttachments(&colorAttachment);

        // Begin dynamic rendering
        commandBuffer.beginRendering(renderingInfo);
    }
    void Render(vk::raii::CommandBuffer& commandBuffer) override {
        // With dynamic rendering, each effect can set up its own rendering state
        // and access input textures directly as shader resources

        // Apply each post-process effect
        for (auto effect : effects) {
            effect->Apply(commandBuffer);
        }
    }
    void EndPass(vk::raii::CommandBuffer& commandBuffer) override {
        // End dynamic rendering
        commandBuffer.endRendering();
    }
};

} // namespace GNC
