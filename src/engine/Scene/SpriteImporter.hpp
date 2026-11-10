#pragma once

#include <filesystem>

namespace GNC {

class Entity;
class ResourceManager;
class Scene;

// Adds an entity with a transform and a sprite of the image file to the
// scene, named after the file. The texture goes into the ResourceManager
// under the file's path relative to the executable, so importing the file
// again, or a scene file, reuses it. Returns the entity, or nullptr when the
// image does not load, which is logged.
Entity* ImportSprite(const std::filesystem::path& path, Scene& scene, ResourceManager& resources);

// Whether the file's extension is an image format sprites are made from.
bool IsImageFile(const std::filesystem::path& path);

} // namespace GNC
