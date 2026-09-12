#include "RenderPassManager.hpp"
#include "RenderPass.hpp"

#include <stdexcept>

namespace GNC {

RenderPass* RenderPassManager::GetRenderPass(const std::string& name) {
    auto it = renderPasses.find(name);
    if (it != renderPasses.end()) {
        return it->second.get();
    }
    return nullptr;
}

void RenderPassManager::RemoveRenderPass(const std::string& name) {
    auto it = renderPasses.find(name);
    if (it != renderPasses.end()) {
        renderPasses.erase(it);
        dirty = true;
    }
}

void RenderPassManager::Execute(vk::raii::CommandBuffer& commandBuffer) {
    if (dirty) {
        SortPasses();
        dirty = false;
    }

    for (auto pass : sortedPasses) {
        pass->Execute(commandBuffer);
    }
}

void RenderPassManager::SortPasses() {
    // Topologically sort render passes based on dependencies
    sortedPasses.clear();

    // Create a copy of render passes for sorting
    std::unordered_map<std::string, RenderPass*> passMap;
    for (const auto& [name, pass] : renderPasses) {
        passMap[name] = pass.get();
    }

    // Perform topological sort
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> visiting;

    for (const auto& [name, pass] : passMap) {
        if (visited.find(name) == visited.end()) {
            TopologicalSort(name, passMap, visited, visiting);
        }
    }
}

void RenderPassManager::TopologicalSort(const std::string& name,
                                        const std::unordered_map<std::string, RenderPass*>& passMap,
                                        std::unordered_set<std::string>& visited,
                                        std::unordered_set<std::string>& visiting) {
    visiting.insert(name);

    auto pass = passMap.at(name);
    for (const auto& dep : pass->GetDependencies()) {
        if (visited.find(dep) == visited.end()) {
            if (visiting.find(dep) != visiting.end()) {
                // Circular dependency detected
                throw std::runtime_error("Circular dependency detected in render passes");
            }
            TopologicalSort(dep, passMap, visited, visiting);
        }
    }

    visiting.erase(name);
    visited.insert(name);
    sortedPasses.push_back(pass);
}

} // namespace GNC
