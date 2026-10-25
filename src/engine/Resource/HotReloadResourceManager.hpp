#pragma once

#include "ResourceManager.hpp"

#include "Utils/Log.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace GNC {

// Hot-reloading resource manager
// Development only: rebuilds a resource when a file behind it changes. The
// watcher thread only stats files; the rebuild happens in
// ApplyPendingReloads() on the main thread, so no Vulkan handle is destroyed
// while a command buffer may still use it.
class HotReloadResourceManager : public ResourceManager {
public:
    using Clock = std::chrono::steady_clock;

    HotReloadResourceManager() = default;
    explicit HotReloadResourceManager(const ResourceContext& ctx) : ResourceManager(ctx) {}

    ~HotReloadResourceManager() override { StopWatcher(); }

    void SetPollInterval(std::chrono::milliseconds interval) {
        pollIntervalMs.store(static_cast<int>(interval.count()), std::memory_order_relaxed);
    }

    // How long a file must hold still before it is reloaded: editors and
    // compilers do not write files atomically.
    void SetSettleTime(std::chrono::milliseconds settle) {
        settleTimeMs.store(static_cast<int>(settle.count()), std::memory_order_relaxed);
    }

    // Also watches the resource's files, from after the load, so the recorded
    // timestamps are the ones the load read.
    template<typename T, typename... Args>
    ResourceHandle<T> Load(const std::string& resourceId, Args&&... args) {
        ResourceHandle<T> handle = ResourceManager::Load<T>(resourceId, std::forward<Args>(args)...);
        if (!handle.IsValid()) return handle;

        if (const T* resource = handle.Get()) {
            Watch(std::type_index(typeid(T)), resourceId, resource->GetSourcePaths());
        }
        return handle;
    }

    // Also stops watching once the resource has left the cache.
    template<typename T>
    void Release(const std::string& resourceId) {
        ResourceManager::Release<T>(resourceId);

        if (!ResourceManager::HasResource<T>(resourceId)) {
            Unwatch(std::type_index(typeid(T)), resourceId);
        }
    }

    void StartWatcher() {
        // exchange(): two racing calls would otherwise both spawn a thread.
        if (running.exchange(true)) return;

        watcher = std::thread(&HotReloadResourceManager::WatcherLoop, this);
    }

    void StopWatcher() {
        if (!running.exchange(false)) return;
        if (watcher.joinable()) watcher.join();
    }

    bool IsWatching() const { return running.load(std::memory_order_relaxed); }

    // Main thread, while no in-flight command buffer uses what it may rebuild
    // (after the frame's fence, or a waitIdle()). A resource that fails to
    // reload keeps its previous version.
    std::size_t ApplyPendingReloads() {
        std::vector<PendingKey> batch;
        {
            std::lock_guard<std::mutex> lock(mutex);
            batch.swap(pending);
        }

        // Outside the lock: ReloadEntry() touches the GPU and must not block
        // the watcher.
        std::size_t reloaded = 0;
        for (const auto& [type, resourceId] : batch) {
            if (ReloadEntry(type, resourceId)) {
                ++reloaded;
                Log::writeFormatted(Log::Info,
                    GREEN_TEXT("INFO::RESOURCE::HOT_RELOAD reloaded '%s'").c_str(),
                    resourceId.c_str());
            } else {
                Log::writeFormatted(Log::Error,
                    RED_TEXT("ERROR::RESOURCE::HOT_RELOAD '%s' failed to reload, "
                             "keeping the previous version").c_str(),
                    resourceId.c_str());
            }
        }
        return reloaded;
    }

    std::size_t GetWatchedFileCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return watched.size();
    }

private:
    struct WatchEntry {
        std::type_index type;
        std::string resourceId;
        std::filesystem::file_time_type stamp{};

        // Watcher thread only, under the lock.
        Clock::time_point changedAt{};
        bool dirty = false;
    };

    using PendingKey = std::pair<std::type_index, std::string>;

    void WatcherLoop() {
        while (running.load(std::memory_order_relaxed)) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                const auto now = Clock::now();
                const auto settle = std::chrono::milliseconds(
                    settleTimeMs.load(std::memory_order_relaxed));

                for (auto& [path, entry] : watched) {
                    std::error_code ec;
                    const auto stamp = std::filesystem::last_write_time(path, ec);

                    // Briefly missing while an editor saves through a temp
                    // file; look again next tick.
                    if (ec) continue;

                    if (stamp != entry.stamp) {
                        // Maybe still being written: restart the settle timer.
                        entry.stamp = stamp;
                        entry.changedAt = now;
                        entry.dirty = true;
                        continue;
                    }

                    // Settled: the writer is done.
                    if (entry.dirty && now - entry.changedAt >= settle) {
                        entry.dirty = false;
                        QueueReload(entry.type, entry.resourceId);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs.load(std::memory_order_relaxed)));
        }
    }

    // Caller holds the lock.
    void QueueReload(std::type_index type, const std::string& resourceId) {
        // One rebuild covers all of a resource's files.
        for (const auto& [queuedType, queuedId] : pending) {
            if (queuedType == type && queuedId == resourceId) return;
        }
        pending.emplace_back(type, resourceId);
    }

    void Watch(std::type_index type, const std::string& resourceId,
               const std::vector<std::string>& paths) {
        std::lock_guard<std::mutex> lock(mutex);

        for (const auto& path : paths) {
            std::error_code ec;
            const auto stamp = std::filesystem::last_write_time(path, ec);

            // Not a file on disk, or not there yet: nothing to watch.
            if (ec) continue;

            watched.insert_or_assign(path, WatchEntry{type, resourceId, stamp, Clock::now(), false});
        }
    }

    void Unwatch(std::type_index type, const std::string& resourceId) {
        std::lock_guard<std::mutex> lock(mutex);

        std::erase_if(watched, [&](const auto& entry) {
            return entry.second.type == type && entry.second.resourceId == resourceId;
        });

        // Its queued reload too: ReloadEntry() would not find it.
        std::erase_if(pending, [&](const PendingKey& key) {
            return key.first == type && key.second == resourceId;
        });
    }

    std::unordered_map<std::string, WatchEntry> watched; // keyed by file path
    std::vector<PendingKey> pending;
    mutable std::mutex mutex;

    std::thread watcher;
    std::atomic<bool> running{false};

    // Atomic so the setters are safe to call while the watcher is running.
    std::atomic<int> pollIntervalMs{500};
    std::atomic<int> settleTimeMs{250};
};

} // namespace GNC
