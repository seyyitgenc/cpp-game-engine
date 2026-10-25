#pragma once

#include "ResourceContext.hpp"

#include <string>
#include <utility>
#include <vector>

namespace GNC {

// Base resource class
// Load() and Unload() keep the loaded flag; subclasses implement
// doLoad()/doUnload() and never touch it.
class Resource {
private:
    std::string resourceId;
    bool loaded = false;

    // Set by the manager before the first Load().
    const ResourceContext* context = nullptr;

public:
    explicit Resource(std::string id) : resourceId(std::move(id)) {}

    // No Unload() here: the derived part is already gone. vk::raii members
    // destroy themselves.
    virtual ~Resource() = default;

    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    const std::string& GetId() const { return resourceId; }
    bool IsLoaded() const { return loaded; }

    void SetContext(const ResourceContext* ctx) { context = ctx; }

    bool Load() {
        if (loaded) return true;

        loaded = doLoad();
        return loaded;
    }

    void Unload() {
        if (!loaded) return;

        doUnload();
        loaded = false;
    }

    // For the hot-reload watcher. Reported by the resource because a path
    // does not map back to an id (Shader folds its stage into the file name).
    virtual std::vector<std::string> GetSourcePaths() const { return {}; }

protected:
    const ResourceContext& GetContext() const { return *context; }
    const vk::raii::Device& GetDevice() const { return *context->device; }

    virtual bool doLoad() = 0;
    virtual void doUnload() = 0;
};

} // namespace GNC
