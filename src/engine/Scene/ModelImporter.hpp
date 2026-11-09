#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace GNC {

class Entity;
class Mesh;
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

// The mesh ImportModel() makes of the file, without entities or materials:
// what a scene file needs. Reused if already loaded; nullptr on failure,
// logged.
Mesh* LoadModelMesh(const std::filesystem::path& path, ResourceManager& resources);

// The encoded images the model file embeds under these names ("*0", or a
// file name for FBX), each nullopt where there is none. Reads the file once.
std::vector<std::optional<std::vector<std::uint8_t>>> ReadEmbeddedImages(const std::filesystem::path& path, const std::vector<std::string>& names);

// Whether assimp reads files with this one's extension.
bool IsModelFile(const std::filesystem::path& path);

} // namespace GNC
