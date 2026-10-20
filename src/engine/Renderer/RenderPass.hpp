#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "FrameData.hpp"

namespace GNC {

// The shader modules a pass builds its pipeline from. Non-owning, and only read
// during the pass's constructor: a module can be destroyed as soon as the
// pipeline that uses it exists.
struct PassShaders {
    vk::ShaderModule vertex;
    vk::ShaderModule fragment;
};

// Base render pass class
class RenderPass {
private:
    std::string name;
    std::vector<std::string> dependencies;
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

    void SetEnabled(bool isEnabled) {
        enabled = isEnabled;
    }

    bool IsEnabled() const {
        return enabled;
    }

    virtual void Execute(const FrameContext& frame) {
        if (!enabled) return;

        BeginPass(frame);
        Render(frame);
        EndPass(frame);
    }

protected:
    // With dynamic rendering, BeginPass typically calls vkCmdBeginRendering
    // instead of vkCmdBeginRenderPass
    virtual void BeginPass(const FrameContext& frame) = 0;
    virtual void Render(const FrameContext& frame) = 0;
    // With dynamic rendering, EndPass typically calls vkCmdEndRendering
    // instead of vkCmdEndRenderPass
    virtual void EndPass(const FrameContext& frame) = 0;
};

} // namespace GNC