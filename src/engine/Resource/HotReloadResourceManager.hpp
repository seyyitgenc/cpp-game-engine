#pragma once

#include "ResourceManager.hpp"

#include "util/log.hpp"

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
// A development-time ResourceManager that notices when a file behind a loaded
// resource changes on disk and rebuilds that resource from it.
//
// The work is split across two threads on purpose:
//
//   - the watcher thread only ever *stats files*. It touches no Vulkan object
//     and no resource table entry, so it cannot race the renderer.
//   - ApplyPendingReloads(), which you call yourself from the main thread,
//     does the actual destroy-and-rebuild.
//
// That split is the whole point. Reloading from the watcher thread, the way the
// obvious implementation does, destroys vk::raii handles at an arbitrary moment
// which may be while a command buffer referencing them is still in flight, and
// races the main thread's Load()/Release() over the same containers.
//
// Reloading is not free of synchronisation duty even so: see
// ApplyPendingReloads() for what the caller still owns.
//
// Ship this out of release builds, or just never call StartWatcher() there.
class HotReloadResourceManager : public ResourceManager {
public:
    using Clock = std::chrono::steady_clock;

    HotReloadResourceManager() = default;
    explicit HotReloadResourceManager(const ResourceContext& ctx) : ResourceManager(ctx) {}

    ~HotReloadResourceManager() override { StopWatcher(); }

    // How often the watcher stats its files.
    void SetPollInterval(std::chrono::milliseconds interval) {
        pollIntervalMs.store(static_cast<int>(interval.count()), std::memory_order_relaxed);
    }

    // How long a file must hold still after a change before it is reloaded.
    // Editors and shader compilers do not write atomically, so reacting to the
    // first changed timestamp regularly means reading a half-written file.
    void SetSettleTime(std::chrono::milliseconds settle) {
        settleTimeMs.store(static_cast<int>(settle.count()), std::memory_order_relaxed);
    }

    // Same contract as ResourceManager::Load(), and additionally registers the
    // resource's source files with the watcher.
    //
    // Registering *after* the load rather than before means the timestamps
    // recorded are the ones the resource actually read, so an edit that lands
    // during startup is caught on the next poll instead of being missed.
    template<typename T, typename... Args>
    ResourceHandle<T> Load(const std::string& resourceId, Args&&... args) {
        ResourceHandle<T> handle = ResourceManager::Load<T>(resourceId, std::forward<Args>(args)...);
        if (!handle.IsValid()) return handle;

        if (const T* resource = handle.Get()) {
            Watch(std::type_index(typeid(T)), resourceId, resource->GetSourcePaths());
        }
        return handle;
    }

    // Same contract as ResourceManager::Release(), and stops watching the files
    // once the resource has actually left the cache.
    template<typename T>
    void Release(const std::string& resourceId) {
        ResourceManager::Release<T>(resourceId);

        if (!ResourceManager::HasResource<T>(resourceId)) {
            Unwatch(std::type_index(typeid(T)), resourceId);
        }
    }

    void StartWatcher() {
        // exchange() rather than a load/store pair: two calls racing here would
        // otherwise both spawn a thread and leak one of them.
        if (running.exchange(true)) return;

        watcher = std::thread(&HotReloadResourceManager::WatcherLoop, this);
    }

    void StopWatcher() {
        if (!running.exchange(false)) return;
        if (watcher.joinable()) watcher.join();
    }

    bool IsWatching() const { return running.load(std::memory_order_relaxed); }

    // Applies every reload the watcher has queued and returns how many resources
    // were rebuilt. Cheap when nothing changed: one mutex acquisition.
    //
    // Call it from the main thread at a point where nothing this could rebuild
    // is referenced by an in-flight command buffer. Between frames, after the
    // fence for the frame you just submitted, is the natural spot; a
    // device.waitIdle() beforehand is the blunt version and perfectly fine for a
    // development-only path.
    //
    // A resource whose new file fails to load keeps its previous version and
    // logs. That is what makes the edit-save-look loop usable: a typo in a
    // shader leaves the last good module bound instead of taking the frame down,
    // and the next save retries.
    std::size_t ApplyPendingReloads() {
        std::vector<PendingKey> batch;
        {
            std::lock_guard<std::mutex> lock(mutex);
            batch.swap(pending);
        }

        // Deliberately outside the lock: ReloadEntry() touches the GPU, and the
        // watcher thread must not be blocked on it.
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

    // Number of files currently being watched. Handy in a debug overlay.
    std::size_t GetWatchedFileCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return watched.size();
    }

private:
    struct WatchEntry {
        std::type_index type;
        std::string resourceId;
        std::filesystem::file_time_type stamp{};

        // When the most recent change was seen, and whether one is still
        // settling. Both only ever touched by the watcher thread under the lock.
        Clock::time_point changedAt{};
        bool dirty = false;
    };

    // Which resource to rebuild, in the type-erased form the table is keyed by.
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

                    // The file can be legitimately missing for an instant: many
                    // editors save by writing a temp file and renaming over the
                    // target. Not an error, just look again next tick.
                    if (ec) continue;

                    if (stamp != entry.stamp) {
                        // Something wrote to it, but possibly is still writing.
                        // Restart the settle timer rather than reloading now.
                        entry.stamp = stamp;
                        entry.changedAt = now;
                        entry.dirty = true;
                        continue;
                    }

                    // Unchanged since last tick and the settle window has passed,
                    // so the writer is done.
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
        // A resource can be built from several files, and a rebuild covers all
        // of them, so collapse duplicates. The queue is drained every frame and
        // holds only what changed, so a linear scan is the right shape here.
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

            // A resource can load from somewhere that is not a file on disk, or
            // name a path that does not exist yet. Nothing to watch either way.
            if (ec) continue;

            watched.insert_or_assign(path, WatchEntry{type, resourceId, stamp, Clock::now(), false});
        }
    }

    void Unwatch(std::type_index type, const std::string& resourceId) {
        std::lock_guard<std::mutex> lock(mutex);

        std::erase_if(watched, [&](const auto& entry) {
            return entry.second.type == type && entry.second.resourceId == resourceId;
        });

        // Drop any queued reload too: the resource is gone, and ReloadEntry()
        // would only fail to find it.
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
