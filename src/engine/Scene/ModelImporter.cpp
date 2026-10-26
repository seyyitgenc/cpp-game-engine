#include "ModelImporter.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ECS/Component/BoundingVolumeComponent.hpp"
#include "ECS/Component/LightComponent.hpp"
#include "ECS/Component/MeshComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Renderer/Material.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Types/Mesh.hpp"
#include "Resource/Types/Texture.hpp"
#include "Scene/Scene.hpp"
#include "Utils/FileSystem.hpp"
#include "Utils/Log.hpp"

namespace GNC {

namespace {

// FlipUVs puts v's origin at the top of the image, as glTF and Vulkan have
// it; assimp's glTF importer flips v to OpenGL's bottom-left origin.
// Missing normals and tangents are made below as glTF specifies, not with
// CalcTangentSpace, whose bitangents run along +v, against glTF's.
constexpr unsigned int PostProcessFlags = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices
                                        | aiProcess_FlipUVs | aiProcess_ValidateDataStructure;

// GL's enums, which glTF samplers use and assimp passes on.
constexpr int GlNearest = 9728;
constexpr int GlLinear = 9729;
constexpr int GlNearestMipmapNearest = 9984;
constexpr int GlLinearMipmapNearest = 9985;
constexpr int GlNearestMipmapLinear = 9986;

// The file's path under models/, so ids do not depend on where the
// executable is.
std::string ModelKey(const std::filesystem::path& path) {
    std::error_code error;
    const std::filesystem::path relative = std::filesystem::relative(path, FileSystem::getPath("models"), error);
    if (error || relative.empty() || *relative.begin() == "..") return path.filename().string();
    return relative.generic_string();
}

// assimp's strings are UTF-8.
std::filesystem::path Utf8Path(std::string_view text) {
    return std::u8string_view(reinterpret_cast<const char8_t*>(text.data()), text.size());
}

std::string Utf8String(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

std::string DecodePercent(std::string_view text) {
    std::string decoded;
    decoded.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        unsigned int byte = 0;
        const char* digits = text.data() + i + 1;
        if (text[i] == '%' && i + 2 < text.size() && std::from_chars(digits, digits + 2, byte, 16).ptr == digits + 2) {
            decoded.push_back(static_cast<char>(byte));
            i += 2;
        } else {
            decoded.push_back(text[i]);
        }
    }
    return decoded;
}

glm::vec3 ToVec3(const aiVector3D& vector) { return { vector.x, vector.y, vector.z }; }
glm::vec3 ToVec3(const aiColor3D& color) { return { color.r, color.g, color.b }; }

// aiMatrix4x4 is row-major.
glm::mat4 ToMat4(const aiMatrix4x4& matrix) { return glm::transpose(glm::make_mat4(&matrix.a1)); }

vk::SamplerAddressMode ToAddressMode(aiTextureMapMode mode) {
    switch (mode) {
        case aiTextureMapMode_Clamp: return vk::SamplerAddressMode::eClampToEdge;
        case aiTextureMapMode_Mirror: return vk::SamplerAddressMode::eMirroredRepeat;
        default: return vk::SamplerAddressMode::eRepeat;
    }
}

// Only glTF files have filters. Without one, glTF leaves filtering to the
// implementation: trilinear.
TextureSettings MakeSettings(const aiMaterial& material, aiTextureType type, const aiTextureMapMode (&modes)[3], bool srgb) {
    TextureSettings settings { .srgb = srgb, .wrapU = ToAddressMode(modes[0]), .wrapV = ToAddressMode(modes[1]) };

    int filter = 0;
    if (material.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MAG(type, 0), filter) == AI_SUCCESS && filter == GlNearest) {
        settings.magFilter = vk::Filter::eNearest;
    }
    if (material.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MIN(type, 0), filter) != AI_SUCCESS) return settings;

    switch (filter) {
        case GlNearest:
            settings.minFilter = vk::Filter::eNearest;
            settings.mipmaps = false;
            break;
        case GlLinear:
            settings.mipmaps = false;
            break;
        case GlNearestMipmapNearest:
            settings.minFilter = vk::Filter::eNearest;
            settings.mipmapMode = vk::SamplerMipmapMode::eNearest;
            break;
        case GlLinearMipmapNearest:
            settings.mipmapMode = vk::SamplerMipmapMode::eNearest;
            break;
        case GlNearestMipmapLinear:
            settings.minFilter = vk::Filter::eNearest;
            break;
        default:
            break;
    }
    return settings;
}

// The first of types the material has a texture for.
std::optional<aiTextureType> FindTexture(const aiMaterial& material, std::initializer_list<aiTextureType> types) {
    for (const aiTextureType type : types) {
        if (material.GetTextureCount(type) > 0) return type;
    }
    return std::nullopt;
}

// glTF asks for flat normals where a primitive has none, so every triangle
// gets three vertices of its own.
void MakeFlatNormals(MeshData& meshData) {
    std::vector<Vertex> flat;
    flat.reserve(meshData.indices.size());
    for (std::size_t i = 0; i + 2 < meshData.indices.size(); i += 3) {
        Vertex corners[3] = {
            meshData.vertices[meshData.indices[i]],
            meshData.vertices[meshData.indices[i + 1]],
            meshData.vertices[meshData.indices[i + 2]]
        };
        const glm::vec3 cross = glm::cross(corners[1].position - corners[0].position, corners[2].position - corners[0].position);
        const glm::vec3 normal = glm::dot(cross, cross) > 0.0f ? glm::normalize(cross) : glm::vec3(0.0f, 1.0f, 0.0f);
        for (Vertex& corner : corners) {
            corner.normal = normal;
            flat.push_back(corner);
        }
    }
    meshData.vertices = std::move(flat);
    meshData.indices.resize(meshData.vertices.size());
    std::iota(meshData.indices.begin(), meshData.indices.end(), 0u);
}

class Importer {
private:
    const std::filesystem::path& path;
    const std::string key;
    const aiScene& model;
    Scene& scene;
    ResourceManager& resources;

    // By assimp material index, made when a mesh first uses one.
    std::vector<Material*> materials;
    // By node name, which is how assimp ties a light to its node.
    std::unordered_map<std::string, const aiLight*> lights;
    std::size_t entityCount = 0;
    std::size_t meshCount = 0;
    std::size_t textureCount = 0;
    // Reported once each, at the end.
    std::set<std::string> unsupported;

public:
    Importer(const std::filesystem::path& file, const aiScene& imported, Scene& target, ResourceManager& resourceManager)
        : path(file), key(ModelKey(file)), model(imported), scene(target), resources(resourceManager),
          materials(imported.mNumMaterials, nullptr) {
        for (unsigned int i = 0; i < model.mNumLights; ++i) {
            lights.emplace(model.mLights[i]->mName.C_Str(), model.mLights[i]);
        }
    }

    Entity* Run(std::chrono::steady_clock::time_point start) {
        // assimp's root is the file's lone root node, or one it adds above
        // several.
        Entity& root = ImportNode(*model.mRootNode, nullptr, path.stem().string());

        if (model.HasAnimations()) unsupported.insert("animations");
        if (model.HasCameras()) unsupported.insert("cameras");
        for (const std::string& feature : unsupported) {
            Log::write(Log::Warning, "Model '", key, "': ignored ", feature, "\n");
        }

        const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
        Log::write(Log::Info, "Model '", key, "': ", entityCount, " entities, ", meshCount, " meshes and ",
            textureCount, " textures loaded in ", seconds, " s\n");
        return &root;
    }

private:
    // ---------
    // Materials
    // ---------
    Material* GetMaterial(unsigned int index) {
        if (index >= materials.size()) return nullptr;
        if (!materials[index]) materials[index] = ImportMaterial(*model.mMaterials[index]);
        return materials[index];
    }

    // glTF's metallic-roughness comes through assimp unchanged. Phong
    // materials (OBJ, Collada, most FBX) become dielectrics: a diffuse color
    // only stands in for a missing texture (Blender writes Kd next to map_Kd
    // without meaning it as a tint), and shininess maps to roughness the
    // way Blender and assimp's glTF importer convert between the two.
    Material* ImportMaterial(const aiMaterial& source) {
        Material material;

        material.baseColorTexture = ImportTexture(source, FindTexture(source, { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE }), true);

        const std::optional<aiTextureType> metallicRoughness = FindTexture(source, { aiTextureType_GLTF_METALLIC_ROUGHNESS });
        material.metallicRoughnessTexture = ImportTexture(source, metallicRoughness, false);
        if (!metallicRoughness && FindTexture(source, { aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS })) {
            unsupported.insert("separate metalness and roughness maps");
        }

        // OBJ's map_Bump is assimp's height map, but exporters put normal
        // maps there.
        const std::optional<aiTextureType> normal = FindTexture(source, { aiTextureType_NORMALS, aiTextureType_HEIGHT });
        material.normalTexture = ImportTexture(source, normal, false);
        if (normal) source.Get(AI_MATKEY_GLTF_TEXTURE_SCALE(*normal, 0), material.normalScale);

        // glTF's occlusion map arrives as the lightmap, its strength under
        // this key rather than AI_MATKEY_GLTF_TEXTURE_STRENGTH.
        const std::optional<aiTextureType> occlusion = FindTexture(source, { aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP });
        material.occlusionTexture = ImportTexture(source, occlusion, false);
        if (occlusion) source.Get(_AI_MATKEY_TEXTURE_BASE ".strength", *occlusion, 0, material.occlusionStrength);

        material.emissiveTexture = ImportTexture(source, FindTexture(source, { aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR }), true);

        aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
        if (source.Get(AI_MATKEY_BASE_COLOR, baseColor) != AI_SUCCESS) {
            aiColor3D diffuse(1.0f, 1.0f, 1.0f);
            if (!material.baseColorTexture) source.Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
            float opacity = 1.0f;
            source.Get(AI_MATKEY_OPACITY, opacity);
            baseColor = aiColor4D(diffuse.r, diffuse.g, diffuse.b, opacity);
            if (opacity < 1.0f) material.alphaMode = AlphaMode::Blend;
        }
        material.baseColorFactor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);

        material.metallicFactor = 0.0f;
        source.Get(AI_MATKEY_METALLIC_FACTOR, material.metallicFactor);
        float shininess = 0.0f;
        if (source.Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughnessFactor) != AI_SUCCESS
            && source.Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            material.roughnessFactor = 1.0f - std::sqrt(std::clamp(shininess / 1000.0f, 0.0f, 1.0f));
        }

        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        source.Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        material.emissiveFactor = ToVec3(emissive);
        source.Get(AI_MATKEY_EMISSIVE_INTENSITY, material.emissiveStrength);

        aiString alphaMode;
        if (source.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
            const std::string_view mode = alphaMode.C_Str();
            material.alphaMode = mode == "MASK" ? AlphaMode::Mask : mode == "BLEND" ? AlphaMode::Blend : AlphaMode::Opaque;
        }
        source.Get(AI_MATKEY_GLTF_ALPHACUTOFF, material.alphaCutoff);
        if (material.alphaMode == AlphaMode::Blend) unsupported.insert("alpha blending (transparent materials are drawn opaque)");

        int twoSided = 0;
        source.Get(AI_MATKEY_TWOSIDED, twoSided);
        material.doubleSided = twoSided != 0;

        int shading = 0;
        if (source.Get(AI_MATKEY_SHADING_MODEL, shading) == AI_SUCCESS && shading == aiShadingMode_Unlit) {
            unsupported.insert("unlit materials (lit as usual)");
        }
        float glossiness = 0.0f;
        if (source.Get(AI_MATKEY_GLOSSINESS_FACTOR, glossiness) == AI_SUCCESS) {
            unsupported.insert("specular-glossiness materials (drawn with the metallic-roughness values)");
        }
        if (FindTexture(source, { aiTextureType_OPACITY })) unsupported.insert("opacity maps");

        return scene.CreateMaterial(material);
    }

    // --------
    // Textures
    // --------
    // One Texture per image and color space; the sampler of the first
    // material to use the image wins.
    Texture* ImportTexture(const aiMaterial& material, std::optional<aiTextureType> type, bool srgb) {
        if (!type) return nullptr;

        aiString file;
        unsigned int uvIndex = 0;
        aiTextureMapMode modes[3] = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
        if (material.GetTexture(*type, 0, &file, nullptr, &uvIndex, nullptr, nullptr, modes) != AI_SUCCESS) return nullptr;
        if (uvIndex != 0) unsupported.insert("texture coordinate sets past the first (it is used instead)");
        // Collada files carry an identity transform for every texture.
        aiUVTransform transform;
        if (material.Get(_AI_MATKEY_UVTRANSFORM_BASE, *type, 0, transform) == AI_SUCCESS
            && (transform.mScaling != aiVector2D(1.0f, 1.0f) || transform.mTranslation != aiVector2D(0.0f, 0.0f) || transform.mRotation != 0.0f)) {
            unsupported.insert("texture transforms (KHR_texture_transform)");
        }

        const std::string id = key + "/" + file.C_Str() + (srgb ? "" : " (linear)");
        if (Texture* existing = resources.GetResource<Texture>(id)) return existing;

        std::optional<TextureSource> source = ImageSource(file.C_Str());
        if (!source) return nullptr;

        // Texture logs why it failed.
        const ResourceHandle<Texture> handle = resources.Load<Texture>(id, std::move(*source), MakeSettings(material, *type, modes, srgb));
        if (!handle) return nullptr;
        ++textureCount;
        return handle.Get();
    }

    std::optional<TextureSource> ImageSource(const char* file) {
        // "*N" for glTF's embedded images, the file name for FBX's.
        if (const aiTexture* embedded = model.GetEmbeddedTexture(file)) {
            // A height of 0 means pcData holds a file's bytes, not texels.
            if (embedded->mHeight != 0) {
                unsupported.insert("uncompressed embedded textures");
                return std::nullopt;
            }
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(embedded->pcData);
            return std::vector<std::uint8_t>(bytes, bytes + embedded->mWidth);
        }

        // Relative to the model file. Exporters also write absolute paths
        // from the machine they ran on, and assimp keeps glTF URIs
        // percent-encoded: failing the path as written, try the file name
        // next to the model, then the decoded URI.
        const std::filesystem::path directory = path.parent_path();
        const std::filesystem::path written = directory / Utf8Path(file);
        for (const std::filesystem::path& candidate : { written, directory / written.filename(), directory / Utf8Path(DecodePercent(file)) }) {
            std::error_code error;
            if (std::filesystem::is_regular_file(candidate, error)) return candidate;
        }
        // Texture logs that it is missing.
        return written;
    }

    // ------
    // Meshes
    // ------
    Mesh* ImportMesh(unsigned int index) {
        const aiMesh& mesh = *model.mMeshes[index];
        const std::string id = key + "/" + (mesh.mName.length > 0 ? mesh.mName.C_Str() : "mesh") + "/" + std::to_string(index);
        if (Mesh* existing = resources.GetResource<Mesh>(id)) return existing;

        // SortByPType leaves each mesh one kind of primitive.
        if (!(mesh.mPrimitiveTypes & aiPrimitiveType_TRIANGLE)) {
            unsupported.insert("points and lines (skipped)");
            return nullptr;
        }
        if (mesh.HasBones()) unsupported.insert("skins (meshes stay in their bind pose)");
        if (mesh.mNumAnimMeshes > 0) unsupported.insert("morph targets");
        if (mesh.HasVertexColors(0)) unsupported.insert("vertex colors");

        MeshData meshData;
        meshData.vertices.resize(mesh.mNumVertices);
        for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
            Vertex& vertex = meshData.vertices[i];
            vertex.position = ToVec3(mesh.mVertices[i]);
            if (mesh.HasNormals()) vertex.normal = ToVec3(mesh.mNormals[i]);
            if (mesh.HasTextureCoords(0)) vertex.texCoord = glm::vec2(mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y);
        }

        // assimp's glTF importer stores cross(normal, tangent) * w as the
        // bitangent, so the dot recovers w. Without normals, glTF says to
        // ignore the tangents too.
        const bool hasTangents = mesh.HasNormals() && mesh.HasTangentsAndBitangents();
        if (hasTangents) {
            for (unsigned int i = 0; i < mesh.mNumVertices; ++i) {
                Vertex& vertex = meshData.vertices[i];
                const glm::vec3 tangent = ToVec3(mesh.mTangents[i]);
                const float handedness = glm::dot(glm::cross(vertex.normal, tangent), ToVec3(mesh.mBitangents[i])) < 0.0f ? -1.0f : 1.0f;
                vertex.tangent = glm::vec4(tangent, handedness);
            }
        }

        meshData.indices.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3);
        for (unsigned int i = 0; i < mesh.mNumFaces; ++i) {
            const aiFace& face = mesh.mFaces[i];
            if (face.mNumIndices == 3) meshData.indices.insert(meshData.indices.end(), face.mIndices, face.mIndices + 3);
        }
        if (meshData.indices.empty()) return nullptr;

        if (!mesh.HasNormals()) MakeFlatNormals(meshData);
        if (!hasTangents) GenerateTangents(meshData);

        const ResourceHandle<Mesh> handle = resources.Load<Mesh>(id, std::move(meshData));
        if (!handle) return nullptr;
        ++meshCount;
        return handle.Get();
    }

    // -----
    // Nodes
    // -----
    // An entity per node, at the node's local transform, under its parent's.
    Entity& ImportNode(const aiNode& node, Entity* parent, const std::string& name) {
        Entity& entity = AddEntity(name, parent, ToMat4(node.mTransformation));

        // Skipped meshes do not count towards needing children.
        std::vector<std::pair<Mesh*, const aiMesh*>> parts;
        for (unsigned int i = 0; i < node.mNumMeshes; ++i) {
            if (Mesh* mesh = ImportMesh(node.mMeshes[i])) parts.emplace_back(mesh, model.mMeshes[node.mMeshes[i]]);
        }
        for (const auto& [mesh, source] : parts) {
            // An entity holds one mesh, so each of several gets a child,
            // named after its material.
            Entity& target = parts.size() == 1 ? entity : AddEntity(PartName(*source), &entity, glm::mat4(1.0f));
            target.AddComponent<MeshComponent>(mesh, GetMaterial(source->mMaterialIndex));
            target.AddComponent<BoundingVolumeComponent>();
        }

        if (const auto light = lights.find(node.mName.C_Str()); light != lights.end()) ImportLight(*light->second, node, entity);

        for (unsigned int i = 0; i < node.mNumChildren; ++i) {
            ImportNode(*node.mChildren[i], &entity, NodeName(*node.mChildren[i]));
        }
        return entity;
    }

    // KHR_lights_punctual's units and axes survive assimp, which also puts
    // other formats' lights down the node's -Z axis, but it multiplies the
    // intensity into the color. Splitting them again is exact when the
    // color's brightest channel was 1.
    void ImportLight(const aiLight& source, const aiNode& node, Entity& entity) {
        LightType type = LightType::Point;
        switch (source.mType) {
            case aiLightSource_DIRECTIONAL: type = LightType::Directional; break;
            case aiLightSource_POINT: type = LightType::Point; break;
            case aiLightSource_SPOT: type = LightType::Spot; break;
            default:
                unsupported.insert("ambient and area lights");
                return;
        }

        const glm::vec3 color = ToVec3(source.mColorDiffuse);
        const float intensity = std::max({ color.r, color.g, color.b });
        LightComponent* light = entity.AddComponent<LightComponent>(type);
        light->SetColor(intensity > 0.0f ? color / intensity : glm::vec3(1.0f));
        light->SetIntensity(intensity);
        // glTF's range, which only fits assimp's attenuation when infinite.
        float range = 0.0f;
        if (node.mMetaData && node.mMetaData->Get("PBR_LightRange", range)) light->SetRange(range);
        if (type == LightType::Spot) light->SetSpotCone(source.mAngleInnerCone, source.mAngleOuterCone);
    }

    Entity& AddEntity(const std::string& name, Entity* parent, const glm::mat4& transform) {
        Entity& entity = scene.CreateEntity(scene.MakeUniqueName(name, parent), parent);
        entity.AddComponent<TransformComponent>()->SetLocalMatrix(transform);
        ++entityCount;
        return entity;
    }

    // Scene::MakeUniqueName() numbers the repeats.
    static std::string NodeName(const aiNode& node) {
        return node.mName.length > 0 ? node.mName.C_Str() : "Node";
    }

    std::string PartName(const aiMesh& mesh) const {
        aiString name;
        if (model.mMaterials[mesh.mMaterialIndex]->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && name.length > 0) return name.C_Str();
        return mesh.mName.length > 0 ? mesh.mName.C_Str() : "Mesh";
    }
};

} // namespace

Entity* ImportModel(const std::filesystem::path& path, Scene& scene, ResourceManager& resources) {
    const auto start = std::chrono::steady_clock::now();
    const std::string file = Utf8String(path);

    Assimp::Importer importer;
    // Otherwise FBX pivots come in as extra nodes between a node and its
    // parent.
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    // A scene without meshes (only lights, say) is flagged incomplete but
    // still imports.
    const aiScene* model = importer.ReadFile(file, PostProcessFlags);
    if (!model || !model->mRootNode) {
        Log::write(Log::Error, "Model '", file, "': ", importer.GetErrorString(), "\n");
        return nullptr;
    }

    return Importer(path, *model, scene, resources).Run(start);
}

bool IsModelFile(const std::filesystem::path& path) {
    static const Assimp::Importer importer;
    const std::string extension = path.extension().string();
    return !extension.empty() && importer.IsExtensionSupported(extension);
}

} // namespace GNC
