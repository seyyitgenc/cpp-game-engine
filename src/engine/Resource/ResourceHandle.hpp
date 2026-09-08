#pragma once

#include <string>
#include <utility>

// Forward declarations
class ResourceManager;

// Resource handle
// A handle is an id plus the manager that owns the resource, never a raw
// pointer to it. The indirection is the point: the manager can move, reload or
// evict a resource behind the handle's back, and the next Get() simply reports
// what is true now instead of dangling.
//
// Note that a handle is not a smart pointer. Copying one does not take a
// reference; the reference count moves only through ResourceManager::Load() and
// ResourceManager::Release().
template<typename T>
class ResourceHandle {
private:
    std::string resourceId;
    ResourceManager* resourceManager = nullptr;

public:
    ResourceHandle() = default;

    ResourceHandle(std::string id, ResourceManager* manager)
        : resourceId(std::move(id)), resourceManager(manager) {}

    // Defined at the bottom of ResourceManager.hpp, where ResourceManager is a
    // complete type. The two headers refer to each other, so one of the
    // definitions has to come late.
    T* Get() const;

    bool IsValid() const { return Get() != nullptr; }

    const std::string& GetId() const { return resourceId; }

    T* operator->() const { return Get(); }
    T& operator*() const { return *Get(); }
    explicit operator bool() const { return IsValid(); }
};
