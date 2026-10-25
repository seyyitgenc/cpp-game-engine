#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <vulkan/vulkan_raii.hpp>

namespace GNC {

class RenderPass;
class RenderTarget;
struct FrameContext;

// Render pass manager
class RenderPassManager {
private:
    std::unordered_map<std::string, std::unique_ptr<RenderPass>> renderPasses;
    std::vector<RenderPass*> sortedPasses;
    bool dirty = true;

public:
    template<typename T, typename... Args>
    T* AddRenderPass(const std::string& name, Args&&... args) {
        static_assert(std::is_base_of<RenderPass, T>::value, "T must derive from RenderPass");

        auto it = renderPasses.find(name);
        if (it != renderPasses.end()) {
            return dynamic_cast<T*>(it->second.get());
        }

        auto pass = std::make_unique<T>(name, std::forward<Args>(args)...);
        T* passPtr = pass.get();
        renderPasses[name] = std::move(pass);
        dirty = true;

        return passPtr;
    }

    RenderPass* GetRenderPass(const std::string& name);
    void RemoveRenderPass(const std::string& name);
    // Destroys every pass. Their GPU objects go with them, so nothing they
    // recorded may still be in flight.
    void Clear();
    void Execute(const FrameContext& frame);

private:
    void SortPasses();
    void TopologicalSort(const std::string& name,
                         const std::unordered_map<std::string, RenderPass*>& passMap,
                         std::unordered_set<std::string>& visited,
                         std::unordered_set<std::string>& visiting);
};

} // namespace GNC
