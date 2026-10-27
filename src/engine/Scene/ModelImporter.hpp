#pragma once

#include <filesystem>

namespace GNC {

class Entity;
class ResourceManager;
class Scene;

// Imports a model file with assimp (glTF, OBJ, FBX, Collada, ...) into the
// scene as one entity, named after the file, holding one mesh of all the
// file's meshes with a submesh per material; the file's lights become its
// children. The mesh and textures go into the ResourceManager under ids made
// from the file's path, so importing a file again reuses them; materials go
// into the scene, fresh for every import. Returns the entity, or nullptr on
// failure, whose reason is logged, as is anything the file uses that is not
// supported.
Entity* ImportModel(const std::filesystem::path& path, Scene& scene, ResourceManager& resources);

// Whether assimp reads files with this one's extension.
bool IsModelFile(const std::filesystem::path& path);

} // namespace GNC
