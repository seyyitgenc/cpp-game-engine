#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

class RenderTarget;

// Base render pass class
class RenderPass {
private:
    std::string name;
    std::vector<std::string> dependencies;
    RenderTarget* target = nullptr;
    bool enabled = true;

public:
    explicit RenderPass(const std::string& passName) : name(passName) {}
    virtual ~RenderPass() = default;

    const std::string& GetName() const { return name; }

    void AddDependency(const std::string& dependency) {
        dependencies.push_back(dependency);
    }

    const std::vector<std::string>& GetDependencies() const {
        return dependencies;
    }

    void SetRenderTarget(RenderTarget* renderTarget) {
        target = renderTarget;
    }

    RenderTarget* GetRenderTarget() const {
        return target;
    }

    void SetEnabled(bool isEnabled) {
        enabled = isEnabled;
    }

    bool IsEnabled() const {
        return enabled;
    }

    virtual void Execute(vk::raii::CommandBuffer& commandBuffer) {
        if (!enabled) return;

        BeginPass(commandBuffer);
        Render(commandBuffer);
        EndPass(commandBuffer);
    }

protected:
    // With dynamic rendering, BeginPass typically calls vkCmdBeginRendering
    // instead of vkCmdBeginRenderPass
    virtual void BeginPass(vk::raii::CommandBuffer& commandBuffer) = 0;
    virtual void Render(vk::raii::CommandBuffer& commandBuffer) = 0;
    // With dynamic rendering, EndPass typically calls vkCmdEndRendering
    // instead of vkCmdEndRenderPass
    virtual void EndPass(vk::raii::CommandBuffer& commandBuffer) = 0;
};

} // namespace GNC