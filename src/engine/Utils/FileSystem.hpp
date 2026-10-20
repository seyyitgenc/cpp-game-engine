#pragma once

#include <filesystem>

// FileSystem
// Resolves resource paths (shaders/, models/) against the directory the
// executable lives in, so the engine finds them no matter which directory it
// was started from. main() calls init(argv[0]) once, before anything loads.
class FileSystem {
public:
    // argv0 is the path the program was started with: absolute when launched
    // from Explorer or a debugger, relative to the working directory when
    // typed in a shell (./engine.exe). absolute() resolves the relative case
    // against the working directory, which is still the one it started from,
    // so call this first thing in main(). Throws std::filesystem::filesystem_error
    // when the path cannot be resolved.
    //
    // Known gap: a program found through PATH gets a bare name ("engine") in
    // argv[0], which this resolves to the working directory.
    static void init(const char* argv0)
    {
        if (argv0 == nullptr || *argv0 == '\0') {
            root = std::filesystem::current_path();
            return;
        }

        // weakly_canonical drops the "./" and "..", and on Linux follows a
        // symlink to the real executable, next to which the resources live.
        root = std::filesystem::weakly_canonical(std::filesystem::absolute(argv0)).parent_path();
    }

    // Directory of the executable; empty until init() runs.
    static const std::filesystem::path& getRoot() { return root; }

    // getPath("shaders/geometry_pass.vert.spv") -> <exe dir>/shaders/geometry_pass.vert.spv
    static std::filesystem::path getPath(const std::filesystem::path& relativePath) { return root / relativePath; }

private:
    // Empty until init(), so getPath() returns the path unchanged and it
    // resolves against the working directory, as before.
    inline static std::filesystem::path root;
};
