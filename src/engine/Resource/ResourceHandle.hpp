#pragma once

#include <string>
#include <utility>

namespace GNC {

class ResourceManager;

// Resource handle
// An id plus the owning manager, not a pointer, so Get() stays correct when
// the manager reloads or evicts the resource. Copying one takes no reference;
// only Load() and Release() count.
template<typename T>
class ResourceHandle {
private:
    std::string resourceId;
    ResourceManager* resourceManager = nullptr;

public:
    ResourceHandle() = default;

    ResourceHandle(std::string id, ResourceManager* manager)
        : resourceId(std::move(id)), resourceManager(manager) {}

    // Defined in ResourceManager.hpp, where ResourceManager is complete.
    T* Get() const;

    bool IsValid() const { return Get() != nullptr; }

    const std::string& GetId() const { return resourceId; }

    T* operator->() const { return Get(); }
    T& operator*() const { return *Get(); }
    explicit operator bool() const { return IsValid(); }
};

} // namespace GNC
