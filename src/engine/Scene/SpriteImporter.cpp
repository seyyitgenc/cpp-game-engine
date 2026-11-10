#include "SpriteImporter.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

#include "ECS/Component/SpriteComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "ECS/Entity/Entity.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Types/Texture.hpp"
#include "Scene/Scene.hpp"
#include "Utils/FileSystem.hpp"

namespace GNC {

Entity* ImportSprite(const std::filesystem::path& path, Scene& scene, ResourceManager& resources) {
    // A sprite draws its image out to the edges, where Repeat would filter
    // the opposite edge in.
    TextureSettings settings;
    settings.wrapU = vk::SamplerAddressMode::eClampToEdge;
    settings.wrapV = vk::SamplerAddressMode::eClampToEdge;

    // Texture logs why it failed.
    const std::string id = FileSystem::getRelativePath(path).generic_string();
    Texture* texture = resources.Load<Texture>(id, path, settings).Get();
    if (!texture) return nullptr;

    Entity& entity = scene.CreateEntity(scene.MakeUniqueName(path.stem().string()));
    entity.AddComponent<TransformComponent>();
    entity.AddComponent<SpriteComponent>()->SetTexture(texture);
    return &entity;
}

// CMakeLists.txt: TEXTURE_EXTENSIONS
bool IsImageFile(const std::filesystem::path& path) {
    static constexpr std::array<std::string_view, 5> Extensions { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };

    std::string extension = path.extension().string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return std::ranges::find(Extensions, extension) != Extensions.end();
}

} // namespace GNC
