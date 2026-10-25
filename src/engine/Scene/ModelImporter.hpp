#pragma once

#include <filesystem>

namespace GNC {

class Entity;
class ResourceManager;
class Scene;

// Imports a model file with assimp (glTF, OBJ, FBX, Collada, ...) into the
// scene as one root entity, named after the file, holding an entity per node
// at its local transform. A node's meshes and light go on its entity; a node
// with several meshes gets a child entity for each.
// Meshes and textures go into the ResourceManager under ids made from the
// file's path, so importing a file again reuses them; materials go into the
// scene, fresh for every import. Returns the root, or nullptr on failure,
// whose reason is logged, as is anything the file uses that is not
// supported.
Entity* ImportModel(const std::filesystem::path& path, Scene& scene, ResourceManager& resources);

// Whether assimp reads files with this one's extension.
bool IsModelFile(const std::filesystem::path& path);

} // namespace GNC
