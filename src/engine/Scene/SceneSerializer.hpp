#pragma once

#include <filesystem>

namespace GNC {

class ComponentRegistry;
class ResourceManager;
class Scene;

// Scene files
// .gncscene files, as docs/SceneFormat.md describes them. Components are
// read and written through their properties, so a component saves what its
// keyed properties hold, and a type loads only if it is registered and
// default constructible.

// Fills scene, which should be new, from the file, loading the meshes and
// textures it names. false if the file cannot be read or is not a scene
// file, logged; anything wrong inside one is logged and skipped.
bool LoadScene(const std::filesystem::path& path, Scene& scene, ResourceManager& resources, const ComponentRegistry& componentTypes);

// Writes a temporary file and renames it over path, so a failed save leaves
// the old file. false on failure, logged.
bool SaveScene(const std::filesystem::path& path, const Scene& scene, const ComponentRegistry& componentTypes);

} // namespace GNC
