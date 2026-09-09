#pragma once

#include "ResourceContext.hpp"

#include <string>
#include <utility>
#include <vector>

namespace GNC {

// Base resource class
// Every resource is an id plus a loaded/unloaded state. Load() and Unload() are
// non-virtual so the state flag is maintained in exactly one place; subclasses
// fill in doLoad()/doUnload() and never touch the flag themselves.
class Resource {
private:
    std::string resourceId;
    bool loaded = false;

    // Injected by the manager right after construction, before the first Load().
    const ResourceContext* context = nullptr;

public:
    explicit Resource(std::string id) : resourceId(std::move(id)) {}

    // No Unload() call here: doUnload() is pure virtual and the derived part of
    // the object is already gone by the time this runs. With vk::raii members
    // that costs nothing, because each handle destroys itself in reverse
    // declaration order anyway.
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

    // Files this resource was built from, for the hot-reload watcher to stat.
    // The resource reports them rather than the watcher deriving them, because
    // the id -> path mapping is one-way: Shader folds its stage into the name,
    // so "shaders/blinn.frag.spv" cannot be turned back into ("blinn", eFragment)
    // without guessing. A resource that returns nothing is simply never watched.
    virtual std::vector<std::string> GetSourcePaths() const { return {}; }

protected:
    const ResourceContext& GetContext() const { return *context; }
    const vk::raii::Device& GetDevice() const { return *context->device; }

    virtual bool doLoad() = 0;
    virtual void doUnload() = 0;
};

} // namespace GNC
