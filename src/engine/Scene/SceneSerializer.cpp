#include "SceneSerializer.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

#include "ECS/Component/ComponentRegistry.hpp"
#include "ECS/Component/Property.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Renderer/Material.hpp"
#include "Resource/Primitives.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Types/Mesh.hpp"
#include "Resource/Types/Texture.hpp"
#include "Scene/ModelImporter.hpp"
#include "Scene/Scene.hpp"
#include "Utils/FileSystem.hpp"
#include "Utils/Log.hpp"
#include "Utils/Overloaded.hpp"

namespace GNC {

namespace {

// Numbers are floats, as the engine stores them, so they print as the
// shortest text that reads back to the same float: 0.4, not the double
// 0.4000000059604645. ordered_map keeps keys in the order written.
using Json = nlohmann::basic_json<nlohmann::ordered_map, std::vector, std::string, bool, std::int64_t, std::uint64_t, float>;

constexpr std::string_view FormatName = "gncscene";
constexpr int FormatVersion = 1;

// -----
// Enums
// -----
// Written by name, so reordering a C++ enum keeps old files' meaning.
template<typename E>
struct EnumKey {
    E value;
    std::string_view key;
};

constexpr EnumKey<SceneDimension> DimensionKeys[] = {
    { SceneDimension::TwoD, "2D" },
    { SceneDimension::ThreeD, "3D" },
};

constexpr EnumKey<AlphaMode> AlphaModeKeys[] = {
    { AlphaMode::Opaque, "Opaque" },
    { AlphaMode::Mask, "Mask" },
    { AlphaMode::Blend, "Blend" },
};

constexpr EnumKey<vk::SamplerAddressMode> WrapKeys[] = {
    { vk::SamplerAddressMode::eRepeat, "Repeat" },
    { vk::SamplerAddressMode::eMirroredRepeat, "MirroredRepeat" },
    { vk::SamplerAddressMode::eClampToEdge, "ClampToEdge" },
};

constexpr EnumKey<vk::Filter> FilterKeys[] = {
    { vk::Filter::eLinear, "Linear" },
    { vk::Filter::eNearest, "Nearest" },
};

constexpr EnumKey<vk::SamplerMipmapMode> MipmapModeKeys[] = {
    { vk::SamplerMipmapMode::eLinear, "Linear" },
    { vk::SamplerMipmapMode::eNearest, "Nearest" },
};

template<typename E, std::size_t N>
std::string ToKey(const EnumKey<E> (&keys)[N], E value) {
    const auto it = std::ranges::find(keys, value, &EnumKey<E>::value);
    return std::string(it != std::end(keys) ? it->key : keys[0].key);
}

template<typename E, std::size_t N>
std::optional<E> FromKey(const EnumKey<E> (&keys)[N], const Json& json) {
    if (!json.is_string()) return std::nullopt;
    const std::string& text = json.get_ref<const std::string&>();
    const auto it = std::ranges::find(keys, text, &EnumKey<E>::key);
    if (it == std::end(keys)) return std::nullopt;
    return it->value;
}

// ------
// Values
// ------
Json ToJson(const glm::vec3& v) { return Json::array({ v.x, v.y, v.z }); }
Json ToJson(const glm::vec4& v) { return Json::array({ v.x, v.y, v.z, v.w }); }
// x, y, z, w, as glTF writes them; glm's constructor takes w first.
Json ToJson(const glm::quat& q) { return Json::array({ q.x, q.y, q.z, q.w }); }

std::optional<float> ReadFloat(const Json& json) {
    if (!json.is_number()) return std::nullopt;
    return json.get<float>();
}

template<int N>
std::optional<glm::vec<N, float>> ReadVector(const Json& json) {
    if (!json.is_array() || json.size() != N) return std::nullopt;
    glm::vec<N, float> vector;
    for (int i = 0; i < N; ++i) {
        const Json& component = json[static_cast<std::size_t>(i)];
        if (!component.is_number()) return std::nullopt;
        vector[i] = component.get<float>();
    }
    return vector;
}

std::optional<glm::quat> ReadQuat(const Json& json) {
    const std::optional<glm::vec4> xyzw = ReadVector<4>(json);
    if (!xyzw) return std::nullopt;
    return glm::quat(xyzw->w, xyzw->x, xyzw->y, xyzw->z);
}

std::optional<std::uint32_t> ReadCount(const Json& json) {
    if (!json.is_number_integer() || json.get<std::int64_t>() < 0 || json.get<std::int64_t>() > UINT32_MAX) return std::nullopt;
    return json.get<std::uint32_t>();
}

// A string, because many JSON readers hold numbers as doubles, which keep
// only 53 bits.
std::string IdText(std::uint64_t id) { return std::format("{:016x}", id); }

std::optional<std::uint64_t> ReadId(const Json& json) {
    if (!json.is_string()) return std::nullopt;
    const std::string& text = json.get_ref<const std::string&>();
    std::uint64_t id = 0;
    const char* end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, id, 16);
    if (text.empty() || error != std::errc() || stop != end || id == 0) return std::nullopt;
    return id;
}

const Json* Find(const Json& object, std::string_view key) {
    if (!object.is_object()) return nullptr;
    const auto it = object.find(key);
    return it != object.end() ? &*it : nullptr;
}

// Names and paths come from users and model files, which are not always
// valid UTF-8: replace what is not instead of throwing.
std::string Dump(const Json& json) { return json.dump(-1, ' ', false, Json::error_handler_t::replace); }

// --------
// Emitting
// --------
// Two-space indent; an array of plain values (a vector, a list of ids) on
// one line, everything else one value per line, so that diffs stay small.
void Emit(std::string& out, const Json& value, int depth) {
    const auto newline = [&](int indent) {
        out += '\n';
        out.append(static_cast<std::size_t>(indent) * 2, ' ');
    };

    if (value.is_object() && !value.empty()) {
        out += '{';
        bool first = true;
        for (const auto& item : value.items()) {
            if (!first) out += ',';
            first = false;
            newline(depth + 1);
            out += Dump(Json(item.key()));
            out += ": ";
            Emit(out, item.value(), depth + 1);
        }
        newline(depth);
        out += '}';
    } else if (value.is_array() && !value.empty()) {
        const bool nested = std::ranges::any_of(value, [](const Json& element) { return element.is_structured(); });
        out += '[';
        bool first = true;
        for (const Json& element : value) {
            if (!first) out += nested ? "," : ", ";
            first = false;
            if (nested) newline(depth + 1);
            Emit(out, element, depth + 1);
        }
        if (nested) newline(depth);
        out += ']';
    } else {
        out += Dump(value);
    }
}

// ------
// Writer
// ------
class Writer {
private:
    const Scene& scene;
    const ComponentRegistry& componentTypes;
    const std::string& file;

    // Filled as the entities first use them, which gives their order.
    Json meshes = Json::object();
    Json textures = Json::object();
    Json materials = Json::object();
    std::unordered_map<const Material*, std::string> materialIds;
    std::unordered_set<std::string> takenMaterialIds;

    // Whose component is being written: a material is named after the
    // first entity that uses it.
    const Entity* currentEntity = nullptr;

    // Reported once each, at the end.
    std::vector<std::string> problems;

public:
    Writer(const Scene& source, const ComponentRegistry& types, const std::string& fileName)
        : scene(source), componentTypes(types), file(fileName) {}

    Json Write() {
        Json root = Json::object();
        root["format"] = std::string(FormatName);
        root["version"] = FormatVersion;
        root["name"] = scene.GetName();
        root["dimension"] = ToKey(DimensionKeys, scene.GetDimension());
        root["camera"] = scene.GetCamera() ? Json(IdText(scene.GetCamera()->GetId())) : Json(nullptr);

        Json environment = Json::object();
        environment["ambientColor"] = ToJson(scene.GetEnvironment().ambientColor);
        environment["exposure"] = scene.GetEnvironment().exposure;
        root["environment"] = std::move(environment);

        // Placeholders hold the documented key order; the entities fill the
        // tables.
        root["meshes"] = nullptr;
        root["textures"] = nullptr;
        root["materials"] = nullptr;

        Json entities = Json::array();
        for (const Entity* entity : scene.GetEntities()) {
            if (!entity->GetParent()) entities.push_back(WriteEntity(*entity));
        }
        root["meshes"] = std::move(meshes);
        root["textures"] = std::move(textures);
        root["materials"] = std::move(materials);
        root["entities"] = std::move(entities);

        std::ranges::sort(problems);
        const auto [first, last] = std::ranges::unique(problems);
        problems.erase(first, last);
        for (const std::string& problem : problems) {
            Log::write(Log::Warning, "Scene '", file, "': ", problem, "\n");
        }
        return root;
    }

private:
    // --------
    // Entities
    // --------
    Json WriteEntity(const Entity& entity) {
        Json json = Json::object();
        json["id"] = IdText(entity.GetId());
        json["name"] = entity.GetName();
        json["active"] = entity.IsActive();

        Json components = Json::array();
        for (const std::unique_ptr<Component>& component : entity.GetComponents()) {
            currentEntity = &entity;
            components.push_back(WriteComponent(*component));
        }
        for (const std::string& unknown : entity.GetUnknownComponents()) {
            components.push_back(Json::parse(unknown, nullptr, false));
        }
        json["components"] = std::move(components);

        if (!entity.GetChildren().empty()) {
            Json children = Json::array();
            for (const Entity* child : entity.GetChildren()) {
                children.push_back(WriteEntity(*child));
            }
            json["children"] = std::move(children);
        }
        return json;
    }

    Json WriteComponent(const Component& component) {
        Json json = Json::object();
        json["type"] = component.GetTypeKey();

        const ComponentRegistry::Entry* type = componentTypes.Find(component.GetTypeID());
        if (!type) {
            problems.push_back(std::string("component type '") + component.GetTypeKey() + "' is not registered, so its values are not saved");
            return json;
        }

        for (const Property& property : type->properties) {
            if (property.info.key.empty()) continue;

            if (property.count) {
                Json elements = Json::array();
                for (std::size_t element = 0; element < property.count(component); ++element) {
                    elements.push_back(WriteValue(property.get(component, element), property.info));
                }
                json[property.info.key] = std::move(elements);
            } else {
                json[property.info.key] = WriteValue(property.get(component, 0), property.info);
            }
        }
        return json;
    }

    Json WriteValue(const PropertyValue& value, const PropertyInfo& info) {
        return std::visit(Overloaded {
            [](bool v) { return Json(v); },
            [&](int v) {
                if (info.hint == PropertyHint::Enum && v >= 0 && static_cast<std::size_t>(v) < info.enumKeys.size()) {
                    return Json(info.enumKeys[static_cast<std::size_t>(v)]);
                }
                return Json(v);
            },
            [](float v) { return Json(v); },
            [](const glm::vec3& v) { return ToJson(v); },
            [](const glm::quat& v) { return ToJson(v); },
            [&](Resource* v) { return WriteResource(v); },
            [&](Material* v) { return WriteMaterial(v); },
        }, value);
    }

    // ---------
    // Resources
    // ---------
    Json WriteResource(const Resource* resource) {
        if (!resource) return nullptr;
        if (const auto* mesh = dynamic_cast<const Mesh*>(resource)) return WriteMesh(*mesh);
        if (const auto* texture = dynamic_cast<const Texture*>(resource)) return WriteTexture(*texture);

        problems.push_back("resource '" + resource->GetId() + "' is of a type scene files cannot name, so it is saved as none");
        return nullptr;
    }

    Json WriteMesh(const Mesh& mesh) {
        const std::string& id = mesh.GetId();
        if (meshes.contains(id)) return id;

        Json json = std::visit(Overloaded {
            [](std::monostate) { return Json(); },
            [](const SpherePrimitive& sphere) {
                Json entry = Json::object();
                entry["primitive"] = "Sphere";
                entry["radius"] = sphere.radius;
                entry["stacks"] = sphere.stacks;
                entry["slices"] = sphere.slices;
                return entry;
            },
            [](const PlanePrimitive& plane) {
                Json entry = Json::object();
                entry["primitive"] = "Plane";
                entry["size"] = plane.size;
                return entry;
            },
            [](const ModelMesh& model) {
                Json entry = Json::object();
                entry["model"] = model.model;
                return entry;
            },
        }, mesh.GetSource());

        if (json.is_null()) {
            problems.push_back("mesh '" + id + "' was not made from a primitive or a model file, so it is saved as none");
            return nullptr;
        }
        meshes[id] = std::move(json);
        return id;
    }

    Json WriteTexture(const Texture& texture) {
        const std::string& id = texture.GetId();
        if (textures.contains(id)) return id;

        Json json = Json::object();
        if (const auto* path = std::get_if<std::filesystem::path>(&texture.GetSource())) {
            json["image"] = FileSystem::getRelativePath(*path).generic_string();
        } else if (const std::optional<EmbeddedImage>& embedded = texture.GetEmbeddedImage()) {
            json["model"] = embedded->model;
            json["image"] = embedded->name;
        } else {
            problems.push_back("texture '" + id + "' has no file to load it from, so it is saved as none");
            return nullptr;
        }

        const TextureSettings& settings = texture.GetSettings();
        json["colorSpace"] = settings.srgb ? "sRGB" : "Linear";
        json["wrapU"] = ToKey(WrapKeys, settings.wrapU);
        json["wrapV"] = ToKey(WrapKeys, settings.wrapV);
        json["magFilter"] = ToKey(FilterKeys, settings.magFilter);
        json["minFilter"] = ToKey(FilterKeys, settings.minFilter);
        json["mipmapMode"] = ToKey(MipmapModeKeys, settings.mipmapMode);
        json["mipmaps"] = settings.mipmaps;

        textures[id] = std::move(json);
        return id;
    }

    // ---------
    // Materials
    // ---------
    Json WriteMaterial(const Material* material) {
        if (!material) return nullptr;
        if (const auto it = materialIds.find(material); it != materialIds.end()) return it->second;

        const std::string id = UniqueMaterialId(currentEntity ? currentEntity->GetName() : "Material");
        materialIds.emplace(material, id);

        Json json = Json::object();
        json["baseColorFactor"] = ToJson(material->baseColorFactor);
        json["metallicFactor"] = material->metallicFactor;
        json["roughnessFactor"] = material->roughnessFactor;
        json["emissiveFactor"] = ToJson(material->emissiveFactor);
        json["emissiveStrength"] = material->emissiveStrength;

        // An empty slot is left out.
        const auto map = [&](const char* key, const Texture* texture) {
            if (!texture) return;
            Json reference = WriteTexture(*texture);
            if (!reference.is_null()) json[key] = std::move(reference);
        };
        map("baseColorTexture", material->baseColorTexture);
        map("metallicRoughnessTexture", material->metallicRoughnessTexture);
        map("normalTexture", material->normalTexture);
        json["normalScale"] = material->normalScale;
        map("occlusionTexture", material->occlusionTexture);
        json["occlusionStrength"] = material->occlusionStrength;
        map("emissiveTexture", material->emissiveTexture);

        json["alphaMode"] = ToKey(AlphaModeKeys, material->alphaMode);
        json["alphaCutoff"] = material->alphaCutoff;
        json["doubleSided"] = material->doubleSided;

        materials[id] = std::move(json);
        return id;
    }

    // name, or "name 1", "name 2", ..., as Scene::MakeUniqueName numbers
    // entities.
    std::string UniqueMaterialId(const std::string& name) {
        std::string id = name;
        for (int number = 1; takenMaterialIds.contains(id); ++number) {
            id = name + " " + std::to_string(number);
        }
        takenMaterialIds.insert(id);
        return id;
    }
};

// ------
// Reader
// ------
class Reader {
private:
    Scene& scene;
    ResourceManager& resources;
    const ComponentRegistry& componentTypes;
    const std::string& file;

    // By the file's ids. A resource that failed to load is there as null,
    // so references to it are not reported again.
    std::unordered_map<std::string, Mesh*> meshes;
    std::unordered_map<std::string, Texture*> textures;
    std::unordered_map<std::string, Material*> materials;

public:
    Reader(Scene& target, ResourceManager& resourceManager, const ComponentRegistry& types, const std::string& fileName)
        : scene(target), resources(resourceManager), componentTypes(types), file(fileName) {}

    bool Read(const Json& root) {
        const Json* format = Find(root, "format");
        if (!format || !format->is_string() || format->get_ref<const std::string&>() != FormatName) {
            Log::write(Log::Error, "Scene '", file, "': not a scene file (no \"format\": \"gncscene\")\n");
            return false;
        }
        const Json* version = Find(root, "version");
        if (!version || !version->is_number_integer() || version->get<std::int64_t>() < 1) {
            Log::write(Log::Error, "Scene '", file, "': no valid version\n");
            return false;
        }
        // Older versions would be upgraded here, one step at a time.
        if (version->get<std::int64_t>() > FormatVersion) {
            Log::write(Log::Error, "Scene '", file, "': version ", version->get<std::int64_t>(), " is newer than this engine reads (",
                FormatVersion, ")\n");
            return false;
        }

        if (const Json* name = Find(root, "name")) {
            if (name->is_string()) scene.SetName(name->get<std::string>());
            else Warn("the scene's name is not a string");
        }
        if (const Json* dimension = Find(root, "dimension")) {
            if (const std::optional<SceneDimension> value = FromKey(DimensionKeys, *dimension)) scene.SetDimension(*value);
            else Warn("dimension is not \"2D\" or \"3D\"");
        }
        if (const Json* environment = Find(root, "environment")) ReadEnvironment(*environment);

        if (const Json* table = Find(root, "meshes")) ReadMeshes(*table);
        if (const Json* table = Find(root, "textures")) ReadTextures(*table);
        if (const Json* table = Find(root, "materials")) ReadMaterials(*table);

        if (const Json* entities = Find(root, "entities")) {
            if (entities->is_array()) {
                for (const Json& entity : *entities) ReadEntity(entity, nullptr);
            } else {
                Warn("entities is not an array");
            }
        }

        // After the entities, which it names.
        if (const Json* camera = Find(root, "camera"); camera && !camera->is_null()) {
            const std::optional<std::uint64_t> id = ReadId(*camera);
            Entity* entity = id ? scene.FindEntity(*id) : nullptr;
            if (entity) scene.SetCamera(entity);
            else Warn("camera names no entity of the scene");
        }

        for (const auto& item : root.items()) {
            static constexpr std::string_view Known[] = { "format", "version", "name", "dimension", "camera", "environment",
                                                          "meshes", "textures", "materials", "entities" };
            if (std::ranges::find(Known, item.key()) == std::end(Known)) Warn("unknown key '", item.key(), "' skipped");
        }
        return true;
    }

private:
    template<typename... Args>
    void Warn(const Args&... args) {
        Log::write(Log::Warning, "Scene '", file, "': ", args..., "\n");
    }

    // Leaves target as it is unless the key is there with a value of the
    // right type.
    void ReadField(const Json& object, std::string_view key, float& target, std::string_view where) {
        const Json* value = Find(object, key);
        if (!value) return;
        if (const std::optional<float> number = ReadFloat(*value)) target = *number;
        else Warn(where, ": ", key, " is not a number");
    }

    template<int N>
    void ReadField(const Json& object, std::string_view key, glm::vec<N, float>& target, std::string_view where) {
        const Json* value = Find(object, key);
        if (!value) return;
        if (const std::optional<glm::vec<N, float>> vector = ReadVector<N>(*value)) target = *vector;
        else Warn(where, ": ", key, " is not ", N, " numbers");
    }

    void ReadField(const Json& object, std::string_view key, bool& target, std::string_view where) {
        const Json* value = Find(object, key);
        if (!value) return;
        if (value->is_boolean()) target = value->get<bool>();
        else Warn(where, ": ", key, " is not true or false");
    }

    template<typename E, std::size_t N>
    void ReadField(const Json& object, std::string_view key, const EnumKey<E> (&keys)[N], E& target, std::string_view where) {
        const Json* value = Find(object, key);
        if (!value) return;
        if (const std::optional<E> known = FromKey(keys, *value)) target = *known;
        else Warn(where, ": ", key, " has an unknown value");
    }

    void ReadEnvironment(const Json& json) {
        SceneEnvironment environment = scene.GetEnvironment();
        ReadField(json, "ambientColor", environment.ambientColor, "environment");
        ReadField(json, "exposure", environment.exposure, "environment");
        scene.SetEnvironment(environment);
    }

    // ------
    // Meshes
    // ------
    void ReadMeshes(const Json& table) {
        if (!table.is_object()) {
            Warn("meshes is not an object");
            return;
        }
        for (const auto& item : table.items()) {
            const std::string& id = item.key();
            const Json& entry = item.value();
            Mesh* mesh = nullptr;

            const Json* primitive = Find(entry, "primitive");
            const Json* model = Find(entry, "model");
            if (primitive && *primitive == "Sphere") {
                SpherePrimitive sphere;
                ReadField(entry, "radius", sphere.radius, "mesh '" + id + "'");
                if (const Json* stacks = Find(entry, "stacks")) sphere.stacks = std::max(ReadCount(*stacks).value_or(sphere.stacks), 2u);
                if (const Json* slices = Find(entry, "slices")) sphere.slices = std::max(ReadCount(*slices).value_or(sphere.slices), 3u);
                mesh = LoadPrimitive(resources, sphere);
            } else if (primitive && *primitive == "Plane") {
                PlanePrimitive plane;
                ReadField(entry, "size", plane.size, "mesh '" + id + "'");
                mesh = LoadPrimitive(resources, plane);
            } else if (model && model->is_string()) {
                // LoadModelMesh logs why it failed.
                mesh = LoadModelMesh(FileSystem::getPath(model->get<std::string>()), resources);
                if (!mesh) Warn("mesh '", id, "' did not load; what uses it has none");
            } else {
                Warn("mesh '", id, "' names no known primitive or model; what uses it has none");
            }
            meshes[id] = mesh;
        }
    }

    // --------
    // Textures
    // --------
    void ReadTextures(const Json& table) {
        if (!table.is_object()) {
            Warn("textures is not an object");
            return;
        }

        // Images embedded in a model file are read per model, so each file
        // is opened once.
        struct Embedded {
            std::string id;
            std::string name;
            TextureSettings settings;
        };
        std::map<std::string, std::vector<Embedded>> embedded;

        for (const auto& item : table.items()) {
            const std::string& id = item.key();
            const Json& entry = item.value();
            const std::string where = "texture '" + id + "'";
            textures[id] = nullptr;

            TextureSettings settings;
            if (const Json* colorSpace = Find(entry, "colorSpace")) {
                if (*colorSpace == "sRGB" || *colorSpace == "Linear") settings.srgb = *colorSpace == "sRGB";
                else Warn(where, ": colorSpace is not \"sRGB\" or \"Linear\"");
            }
            ReadField(entry, "wrapU", WrapKeys, settings.wrapU, where);
            ReadField(entry, "wrapV", WrapKeys, settings.wrapV, where);
            ReadField(entry, "magFilter", FilterKeys, settings.magFilter, where);
            ReadField(entry, "minFilter", FilterKeys, settings.minFilter, where);
            ReadField(entry, "mipmapMode", MipmapModeKeys, settings.mipmapMode, where);
            ReadField(entry, "mipmaps", settings.mipmaps, where);

            const Json* image = Find(entry, "image");
            const Json* model = Find(entry, "model");
            if (!image || !image->is_string() || (model && !model->is_string())) {
                Warn(where, " names no image");
                continue;
            }

            if (Texture* loaded = resources.GetResource<Texture>(id)) {
                textures[id] = loaded;
            } else if (model) {
                embedded[model->get<std::string>()].push_back({ id, image->get<std::string>(), settings });
            } else {
                // Texture logs why it failed.
                textures[id] = resources.Load<Texture>(id, FileSystem::getPath(image->get<std::string>()), settings).Get();
            }
        }

        for (auto& [model, images] : embedded) {
            std::vector<std::string> names;
            for (const Embedded& image : images) names.push_back(image.name);

            std::vector<std::optional<std::vector<std::uint8_t>>> bytes = ReadEmbeddedImages(FileSystem::getPath(model), names);
            for (std::size_t i = 0; i < images.size(); ++i) {
                if (!bytes[i]) {
                    Warn("texture '", images[i].id, "': model '", model, "' embeds no image '", images[i].name, "'");
                    continue;
                }
                textures[images[i].id] = resources.Load<Texture>(images[i].id, std::move(*bytes[i]), images[i].settings,
                    EmbeddedImage { model, images[i].name }).Get();
            }
        }
    }

    // ---------
    // Materials
    // ---------
    void ReadMaterials(const Json& table) {
        if (!table.is_object()) {
            Warn("materials is not an object");
            return;
        }
        for (const auto& item : table.items()) {
            const std::string where = "material '" + item.key() + "'";
            const Json& entry = item.value();
            if (!entry.is_object()) Warn(where, " is not an object");

            Material material;
            ReadField(entry, "baseColorFactor", material.baseColorFactor, where);
            ReadField(entry, "metallicFactor", material.metallicFactor, where);
            ReadField(entry, "roughnessFactor", material.roughnessFactor, where);
            ReadField(entry, "emissiveFactor", material.emissiveFactor, where);
            ReadField(entry, "emissiveStrength", material.emissiveStrength, where);
            material.baseColorTexture = ReadTextureReference(entry, "baseColorTexture", where);
            material.metallicRoughnessTexture = ReadTextureReference(entry, "metallicRoughnessTexture", where);
            material.normalTexture = ReadTextureReference(entry, "normalTexture", where);
            ReadField(entry, "normalScale", material.normalScale, where);
            material.occlusionTexture = ReadTextureReference(entry, "occlusionTexture", where);
            ReadField(entry, "occlusionStrength", material.occlusionStrength, where);
            material.emissiveTexture = ReadTextureReference(entry, "emissiveTexture", where);
            ReadField(entry, "alphaMode", AlphaModeKeys, material.alphaMode, where);
            ReadField(entry, "alphaCutoff", material.alphaCutoff, where);
            ReadField(entry, "doubleSided", material.doubleSided, where);

            materials[item.key()] = scene.CreateMaterial(material);
        }
    }

    Texture* ReadTextureReference(const Json& object, std::string_view key, std::string_view where) {
        const Json* value = Find(object, key);
        if (!value || value->is_null()) return nullptr;
        const auto it = value->is_string() ? textures.find(value->get<std::string>()) : textures.end();
        if (it == textures.end()) {
            Warn(where, ": ", key, " names no texture of the file");
            return nullptr;
        }
        return it->second;
    }

    // --------
    // Entities
    // --------
    void ReadEntity(const Json& json, Entity* parent) {
        if (!json.is_object()) {
            Warn("an entity is not an object; skipped");
            return;
        }

        std::string name = "Entity";
        if (const Json* value = Find(json, "name"); value && value->is_string()) name = value->get<std::string>();

        // 0: the scene picks one.
        std::uint64_t id = 0;
        if (const Json* value = Find(json, "id")) {
            const std::optional<std::uint64_t> parsed = ReadId(*value);
            if (!parsed) Warn("entity '", name, "': the id is not 16 hex digits; it gets a new one");
            else if (scene.FindEntity(*parsed)) Warn("entity '", name, "' has another's id; it gets a new one");
            else id = *parsed;
        }

        Entity& entity = scene.CreateEntity(scene.MakeUniqueName(name, parent), parent, id);
        bool active = true;
        ReadField(json, "active", active, "entity '" + name + "'");
        entity.SetActive(active);

        if (const Json* components = Find(json, "components")) {
            if (components->is_array()) {
                for (const Json& component : *components) ReadComponent(entity, component);
            } else {
                Warn("entity '", name, "': components is not an array");
            }
        }
        if (const Json* children = Find(json, "children")) {
            if (children->is_array()) {
                for (const Json& child : *children) ReadEntity(child, &entity);
            } else {
                Warn("entity '", name, "': children is not an array");
            }
        }
    }

    void ReadComponent(Entity& entity, const Json& json) {
        const Json* typeKey = Find(json, "type");
        if (!typeKey || !typeKey->is_string()) {
            Warn("entity '", entity.GetName(), "': a component without a type; skipped");
            return;
        }
        const std::string& key = typeKey->get_ref<const std::string&>();
        const std::string where = "entity '" + entity.GetName() + "', " + key;

        // Kept, so saving the scene writes it back.
        const ComponentRegistry::Entry* type = componentTypes.FindByKey(key);
        if (!type || !type->create) {
            Warn(where, ": not a component type this engine loads; kept as it is");
            entity.AddUnknownComponent(Dump(json));
            return;
        }
        if (entity.HasComponent(type->typeID)) {
            Warn(where, ": the entity already has one; skipped");
            return;
        }

        Component& component = *type->create(entity);
        for (const Property& property : type->properties) {
            if (property.info.key.empty()) continue;
            const Json* value = Find(json, property.info.key);
            if (!value) continue;

            if (!property.count) {
                ApplyValue(component, property, 0, *value, where);
            } else if (value->is_array()) {
                for (std::size_t element = 0; element < value->size(); ++element) {
                    ApplyValue(component, property, element, (*value)[element], where);
                }
            } else {
                Warn(where, ": ", property.info.key, " is not an array");
            }
        }

        for (const auto& item : json.items()) {
            if (item.key() == "type") continue;
            const bool known = std::ranges::any_of(type->properties, [&](const Property& property) { return property.info.key == item.key(); });
            if (!known) Warn(where, ": unknown key '", item.key(), "' skipped");
        }
    }

    void ApplyValue(Component& component, const Property& property, std::size_t element, const Json& json, const std::string& where) {
        if (const std::optional<PropertyValue> value = ReadValue(json, property.info, property.prototype, where)) {
            property.set(component, element, *value);
        } else {
            Warn(where, ": ", property.info.key, " has a value of the wrong type; the default is kept");
        }
    }

    // nullopt for a value of the wrong type. A reference to an id the file
    // does not have is reported here and read as null.
    std::optional<PropertyValue> ReadValue(const Json& json, const PropertyInfo& info, const PropertyValue& prototype, const std::string& where) {
        using Result = std::optional<PropertyValue>;
        return std::visit(Overloaded {
            [&](bool) -> Result {
                if (json.is_boolean()) return PropertyValue(json.get<bool>());
                return std::nullopt;
            },
            [&](int) -> Result {
                if (json.is_number_integer()) return PropertyValue(json.get<int>());
                if (!json.is_string()) return std::nullopt;
                const auto it = std::ranges::find(info.enumKeys, json.get_ref<const std::string&>());
                if (it == info.enumKeys.end()) return std::nullopt;
                return PropertyValue(static_cast<int>(it - info.enumKeys.begin()));
            },
            [&](float) -> Result {
                if (const std::optional<float> value = ReadFloat(json)) return PropertyValue(*value);
                return std::nullopt;
            },
            [&](const glm::vec3&) -> Result {
                if (const std::optional<glm::vec3> value = ReadVector<3>(json)) return PropertyValue(*value);
                return std::nullopt;
            },
            [&](const glm::quat&) -> Result {
                if (const std::optional<glm::quat> value = ReadQuat(json)) return PropertyValue(*value);
                return std::nullopt;
            },
            [&](Resource*) -> Result {
                if (json.is_null()) return PropertyValue(static_cast<Resource*>(nullptr));
                if (!json.is_string() || !info.resourceType) return std::nullopt;
                const std::string& id = json.get_ref<const std::string&>();
                Resource* resource = nullptr;
                if (*info.resourceType == typeid(Mesh)) {
                    if (const auto it = meshes.find(id); it != meshes.end()) resource = it->second;
                    else Warn(where, ": ", info.key, " names no mesh of the file");
                } else if (*info.resourceType == typeid(Texture)) {
                    if (const auto it = textures.find(id); it != textures.end()) resource = it->second;
                    else Warn(where, ": ", info.key, " names no texture of the file");
                } else {
                    return std::nullopt;
                }
                return PropertyValue(resource);
            },
            [&](Material*) -> Result {
                if (json.is_null()) return PropertyValue(static_cast<Material*>(nullptr));
                if (!json.is_string()) return std::nullopt;
                const auto it = materials.find(json.get<std::string>());
                if (it != materials.end()) return PropertyValue(it->second);
                Warn(where, ": ", info.key, " names no material of the file");
                return PropertyValue(static_cast<Material*>(nullptr));
            },
        }, prototype);
    }
};

} // namespace

bool LoadScene(const std::filesystem::path& path, Scene& scene, ResourceManager& resources, const ComponentRegistry& componentTypes) {
    const auto start = std::chrono::steady_clock::now();
    const std::string file = path.filename().string();

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        Log::write(Log::Error, "Scene '", path.string(), "': cannot open the file\n");
        return false;
    }

    try {
        const Json root = Json::parse(stream);
        if (!Reader(scene, resources, componentTypes, file).Read(root)) return false;
    } catch (const Json::exception& error) {
        // parse_error says where in the file.
        Log::write(Log::Error, "Scene '", file, "': ", error.what(), "\n");
        return false;
    }

    const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    Log::write(Log::Info, "Scene '", file, "': ", scene.GetEntities().size(), " entities loaded in ", seconds, " s\n");
    return true;
}

bool SaveScene(const std::filesystem::path& path, const Scene& scene, const ComponentRegistry& componentTypes) {
    const std::string file = path.filename().string();

    std::string text;
    try {
        Emit(text, Writer(scene, componentTypes, file).Write(), 0);
    } catch (const Json::exception& error) {
        Log::write(Log::Error, "Scene '", file, "': not saved: ", error.what(), "\n");
        return false;
    }
    text += '\n';

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!stream) {
            Log::write(Log::Error, "Scene '", file, "': cannot write '", temporary.string(), "'\n");
            return false;
        }
    }
    std::filesystem::rename(temporary, path, error);
    if (error) {
        Log::write(Log::Error, "Scene '", file, "': cannot replace the file: ", error.message(), "\n");
        std::filesystem::remove(temporary, error);
        return false;
    }

    Log::write(Log::Info, "Scene '", file, "' saved\n");
    return true;
}

} // namespace GNC
