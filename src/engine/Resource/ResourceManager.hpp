#pragma once

#include "Resource.hpp"
#include "ResourceContext.hpp"
#include "ResourceHandle.hpp"

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace GNC {

// Resource manager
// Owns every resource in the engine, keyed by type and then by id. The two
// levels mean a Texture called "wood" and a Mesh called "wood" never collide,
// and both are an O(1) average lookup.
//
// The reference count is deliberately separate from the shared_ptr's: it counts
// how many times the *game* asked for a resource, not how many pointers happen
// to exist. That is what lets you pin something in memory across a level load
// even while nothing is pointing at it.
//
// FIXME: not thread safe
class ResourceManager {
private:
    struct ResourceData {
        std::shared_ptr<Resource> resource;
        int refCount = 0;

        // Rebuilds this resource from scratch with the same constructor
        // arguments it was first loaded with. Stored because the table is type
        // erased: by reload time the manager no longer knows what T was, and
        // Shader in particular cannot be reconstructed from its id alone.
        std::function<std::shared_ptr<Resource>()> factory;
    };

    using ResourceTable = std::unordered_map<std::string, ResourceData>;

    std::unordered_map<std::type_index, ResourceTable> resources;
    ResourceContext context;

public:
    ResourceManager() = default;
    explicit ResourceManager(const ResourceContext& ctx) : context(ctx) {}

    // Virtual so HotReloadResourceManager can be owned through a base pointer.
    virtual ~ResourceManager() { UnloadAll(); }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    void SetContext(const ResourceContext& ctx) { context = ctx; }
    const ResourceContext& GetContext() const { return context; }

    // Cache first, disk second. Extra arguments are forwarded to T's
    // constructor, which is how Shader gets told which stage it is.
    template<typename T, typename... Args>
    ResourceHandle<T> Load(const std::string& resourceId, Args&&... args) {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        ResourceTable& table = resources[std::type_index(typeid(T))];

        if (auto it = table.find(resourceId); it != table.end()) {
            ++it->second.refCount;
            return ResourceHandle<T>(resourceId, this);
        }

        // Built once and kept, so a later reload can construct an identical
        // replacement without knowing T. Arguments are copied into the closure
        // rather than forwarded, since they have to survive for the manager's
        // lifetime and be usable more than once.
        std::function<std::shared_ptr<Resource>()> factory =
            [id = resourceId, saved = std::make_tuple(args...)]() -> std::shared_ptr<Resource> {
                return std::apply(
                    [&id](const auto&... a) { return std::make_shared<T>(id, a...); }, saved);
            };

        auto resource = factory();
        resource->SetContext(&context);

        // A failed load leaves nothing in the cache, so the next Load() is free
        // to try again once the file is fixed. The caller gets a handle that
        // reports itself invalid rather than an exception.
        if (!resource->Load()) {
            return ResourceHandle<T>();
        }

        table.emplace(resourceId, ResourceData{std::move(resource), 1, std::move(factory)});
        return ResourceHandle<T>(resourceId, this);
    }

    template<typename T>
    T* GetResource(const std::string& resourceId) {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        auto table = resources.find(std::type_index(typeid(T)));
        if (table == resources.end()) return nullptr;

        auto it = table->second.find(resourceId);
        if (it == table->second.end()) return nullptr;

        // The outer key is the exact type, so this cast cannot be wrong.
        return static_cast<T*>(it->second.resource.get());
    }

    template<typename T>
    bool HasResource(const std::string& resourceId) const {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        auto table = resources.find(std::type_index(typeid(T)));
        if (table == resources.end()) return false;

        return table->second.contains(resourceId);
    }

    // Pin a resource that is already loaded, for callers that want to keep it
    // alive without going through Load() again.
    template<typename T>
    void AddRef(const std::string& resourceId) {
        auto table = resources.find(std::type_index(typeid(T)));
        if (table == resources.end()) return;

        if (auto it = table->second.find(resourceId); it != table->second.end()) {
            ++it->second.refCount;
        }
    }

    // The tutorial's Release() takes only an id and has to sweep every type
    // table to find it. Releasing with the type spelled out keeps it O(1) and
    // matches how Load() was called in the first place.
    template<typename T>
    void Release(const std::string& resourceId) {
        auto table = resources.find(std::type_index(typeid(T)));
        if (table == resources.end()) return;

        auto it = table->second.find(resourceId);
        if (it == table->second.end()) return;

        if (--it->second.refCount > 0) return;

        it->second.resource->Unload();
        table->second.erase(it);
    }

    template<typename T>
    int GetRefCount(const std::string& resourceId) const {
        auto table = resources.find(std::type_index(typeid(T)));
        if (table == resources.end()) return 0;

        auto it = table->second.find(resourceId);
        return it == table->second.end() ? 0 : it->second.refCount;
    }

    // Shutdown path: drop everything regardless of reference counts. Must run
    // while the device is still alive, since the GPU handles die with it.
    void UnloadAll() {
        for (auto& [type, table] : resources) {
            for (auto& [id, data] : table) {
                data.resource->Unload();
            }
        }
        resources.clear();
    }

protected:
    // Rebuild one resource in place. Used by the hot-reload watcher, which knows
    // the type only as a type_index by the time a file changes.
    //
    // The replacement is built and loaded *before* the old one is touched, so a
    // shader with a typo in it leaves the last good module running instead of
    // taking the renderer down. Only on success does the slot change hands.
    //
    // Every ResourceHandle survives this: handles store an id, not a pointer, so
    // the next Get() resolves to whatever is in the slot now.
    //
    // Destroys GPU objects, so the caller owns the synchronisation: no command
    // buffer still referencing this resource may be in flight.
    bool ReloadEntry(std::type_index type, const std::string& resourceId) {
        auto table = resources.find(type);
        if (table == resources.end()) return false;

        auto it = table->second.find(resourceId);
        if (it == table->second.end() || !it->second.factory) return false;

        auto replacement = it->second.factory();
        replacement->SetContext(&context);
        if (!replacement->Load()) return false;

        it->second.resource->Unload();
        it->second.resource = std::move(replacement);
        return true;
    }
};

template<typename T>
T* ResourceHandle<T>::Get() const {
    if (!resourceManager) return nullptr;
    return resourceManager->GetResource<T>(resourceId);
}

} // namespace GNC
