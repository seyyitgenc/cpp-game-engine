#pragma once

#include <filesystem>

#include "config.h"

// FileSystem
// Resolves resource paths against the executable's directory, whatever the
// working directory is.
class FileSystem {
public:
    // First thing in main(): a relative argv0 resolves against the working
    // directory it started in. Throws std::filesystem::filesystem_error when
    // it cannot be resolved. A bare name from a PATH lookup resolves to the
    // working directory.
    static void init(const char* argv0)
    {
        if (argv0 == nullptr || *argv0 == '\0') {
            root = std::filesystem::current_path();
            return;
        }

        // Also follows a symlink to the real executable, next to which the
        // resources live.
        root = std::filesystem::weakly_canonical(std::filesystem::absolute(argv0)).parent_path();
    }

    // Empty until init().
    static const std::filesystem::path& getRoot() { return root; }

    // getPath("shaders/geometry_pass.vert.spv") -> <exe dir>/shaders/geometry_pass.vert.spv
    static std::filesystem::path getPath(const std::filesystem::path& relativePath) { return root / relativePath; }

    // The inverse of getPath(): relative to the executable's directory when
    // under it, otherwise unchanged.
    static std::filesystem::path getRelativePath(const std::filesystem::path& path)
    {
        std::error_code error;
        const std::filesystem::path relative = std::filesystem::relative(path, root, error);
        if (error || relative.empty() || *relative.begin() == "..") return path;
        return relative;
    }

    // The source tree's resources/scenes, as configured: the editor opens
    // and saves scene files there in place, and the build does not copy
    // them next to the executable.
    static std::filesystem::path getSceneDirectory() { return std::filesystem::path(logl_root) / "resources" / "scenes"; }

private:
    inline static std::filesystem::path root;
};
