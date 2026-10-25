#pragma once

#include "Resource.hpp"
#include "ResourceContext.hpp"
#include "ResourceHandle.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace GNC {

// Resource manager
// Owns every resource, keyed by type and then id. The reference count is
// separate from the shared_ptr's: it counts Load() calls, so a resource stays
// pinned even while nothing points at it.
//
// FIXME: not thread safe
class ResourceManager {
private:
    struct ResourceData {
        std::shared_ptr<Resource> resource;
        int refCount = 0;

        // Rebuilds it with its original constructor arguments: by reload
        // time the type-erased table no longer knows T.
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

    // Cache first, disk second. Extra arguments go to T's constructor (a
    // Shader's stage).
    template<typename T, typename... Args>
    ResourceHandle<T> Load(const std::string& resourceId, Args&&... args) {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        ResourceTable& table = resources[std::type_index(typeid(T))];

        if (auto it = table.find(resourceId); it != table.end()) {
            ++it->second.refCount;
            return ResourceHandle<T>(resourceId, this);
        }

        // Arguments copied, not forwarded: a reload uses them again.
        std::function<std::shared_ptr<Resource>()> factory =
            [id = resourceId, saved = std::make_tuple(args...)]() -> std::shared_ptr<Resource> {
                return std::apply(
                    [&id](const auto&... a) { return std::make_shared<T>(id, a...); }, saved);
            };

        auto resource = factory();
        resource->SetContext(&context);

        // Cached only on success, so the next Load() retries.
        if (!resource->Load()) {
            return ResourceHandle<T>();
        }

        table.emplace(resourceId, ResourceData{std::move(resource), 1, std::move(factory)});
        return ResourceHandle<T>(resourceId, this);
    }

    template<typename T>
    T* GetResource(const std::string& resourceId) {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        // The outer key is the exact type, so this cast cannot be wrong.
        return static_cast<T*>(GetResource(std::type_index(typeid(T)), resourceId));
    }

    Resource* GetResource(std::type_index type, const std::string& resourceId) {
        auto table = resources.find(type);
        if (table == resources.end()) return nullptr;

        auto it = table->second.find(resourceId);
        if (it == table->second.end()) return nullptr;

        return it->second.resource.get();
    }

    template<typename T>
    bool HasResource(const std::string& resourceId) const {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        auto table = resources.find(std::type_index(typeid(T)));
        if (table == resources.end()) return false;

        return table->second.contains(resourceId);
    }

    // Every loaded T's id, sorted.
    template<typename T>
    std::vector<std::string> GetResourceIds() const {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        return GetResourceIds(std::type_index(typeid(T)));
    }

    std::vector<std::string> GetResourceIds(std::type_index type) const {
        std::vector<std::string> ids;
        auto table = resources.find(type);
        if (table == resources.end()) return ids;

        for (const auto& [id, data] : table->second) {
            ids.push_back(id);
        }
        std::ranges::sort(ids);
        return ids;
    }

    // Pins a loaded resource without another Load().
    template<typename T>
    void AddRef(const std::string& resourceId) {
        auto table = resources.find(std::type_index(typeid(T)));
        if (table == resources.end()) return;

        if (auto it = table->second.find(resourceId); it != table->second.end()) {
            ++it->second.refCount;
        }
    }

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

    // Ignores reference counts. Must run while the device is still alive.
    void UnloadAll() {
        for (auto& [type, table] : resources) {
            for (auto& [id, data] : table) {
                data.resource->Unload();
            }
        }
        resources.clear();
    }

protected:
    // For hot reload. The replacement loads before the old one is touched, so
    // a broken file keeps the last good version; handles store ids, so they
    // follow. Destroys GPU objects: nothing using them may be in flight.
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
