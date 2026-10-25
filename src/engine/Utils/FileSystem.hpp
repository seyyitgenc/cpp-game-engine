#pragma once

#include <filesystem>

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

private:
    inline static std::filesystem::path root;
};
