#pragma once

#include "Component.hpp"
#include "Property.hpp"

#include <algorithm>
#include <typeinfo>

#include <glm/glm.hpp>

#include "../../Resource/Types/Texture.hpp"

namespace GNC {

// Sprite component
// A textured rectangle in the entity's XY plane, centered on its origin,
// which SpritePass draws unlit and alpha blended. It only describes what to
// draw. Without a texture it is a white square, one unit a side.
class SpriteComponent : public Component {
public:
    static constexpr float MinPixelsPerUnit = 0.01f;

private:
    Texture* texture = nullptr;
    // Multiplied with the texture's. Linear RGB.
    glm::vec3 color {1.0f};
    float opacity = 1.0f;
    // Texture pixels to a unit of the entity's space.
    float pixelsPerUnit = 100.0f;
    bool flipX = false;
    bool flipY = false;
    // A higher layer draws over a lower one, whatever their depths.
    int layer = 0;

public:
    GNC_COMPONENT_TYPE(SpriteComponent, "Sprite", "Sprite")

    void SetTexture(Texture* value) { texture = value; }
    void SetColor(const glm::vec3& linearColor) { color = linearColor; }
    void SetOpacity(float value) { opacity = std::clamp(value, 0.0f, 1.0f); }
    void SetPixelsPerUnit(float value) { pixelsPerUnit = std::max(value, MinPixelsPerUnit); }
    void SetFlipX(bool flip) { flipX = flip; }
    void SetFlipY(bool flip) { flipY = flip; }
    void SetLayer(int value) { layer = value; }

    Texture* GetTexture() const { return texture; }
    const glm::vec3& GetColor() const { return color; }
    float GetOpacity() const { return opacity; }
    float GetPixelsPerUnit() const { return pixelsPerUnit; }
    bool GetFlipX() const { return flipX; }
    bool GetFlipY() const { return flipY; }
    int GetLayer() const { return layer; }

    // Of the rectangle, in the entity's units. What draws and what culls it
    // both center it on the origin.
    glm::vec2 GetSize() const {
        // 0 until the texture has loaded.
        if (!texture || texture->GetWidth() == 0) return glm::vec2(1.0f);
        return glm::vec2(texture->GetWidth(), texture->GetHeight()) / pixelsPerUnit;
    }

    static void Reflect(PropertyList<SpriteComponent>& properties) {
        properties.Add<Resource*>(
            { .name = "Texture", .key = "texture", .resourceType = &typeid(Texture) },
            [](const SpriteComponent& sprite) -> Resource* { return sprite.GetTexture(); },
            [](SpriteComponent& sprite, Resource* value) { sprite.SetTexture(dynamic_cast<Texture*>(value)); });

        properties.Add<glm::vec3>(
            { .name = "Color", .key = "color", .hint = PropertyHint::Color },
            [](const SpriteComponent& sprite) { return sprite.GetColor(); },
            [](SpriteComponent& sprite, const glm::vec3& value) { sprite.SetColor(value); });

        properties.Add<float>(
            { .name = "Opacity", .key = "opacity", .min = 0.0f, .max = 1.0f, .step = 0.005f },
            [](const SpriteComponent& sprite) { return sprite.GetOpacity(); },
            [](SpriteComponent& sprite, float value) { sprite.SetOpacity(value); });

        properties.Add<float>(
            { .name = "Pixels Per Unit", .key = "pixelsPerUnit", .min = MinPixelsPerUnit, .step = 1.0f, .format = "%.2f" },
            [](const SpriteComponent& sprite) { return sprite.GetPixelsPerUnit(); },
            [](SpriteComponent& sprite, float value) { sprite.SetPixelsPerUnit(value); });

        properties.Add<bool>(
            { .name = "Flip X", .key = "flipX" },
            [](const SpriteComponent& sprite) { return sprite.GetFlipX(); },
            [](SpriteComponent& sprite, bool value) { sprite.SetFlipX(value); });

        properties.Add<bool>(
            { .name = "Flip Y", .key = "flipY" },
            [](const SpriteComponent& sprite) { return sprite.GetFlipY(); },
            [](SpriteComponent& sprite, bool value) { sprite.SetFlipY(value); });

        properties.Add<int>(
            { .name = "Layer", .key = "layer" },
            [](const SpriteComponent& sprite) { return sprite.GetLayer(); },
            [](SpriteComponent& sprite, int value) { sprite.SetLayer(value); });
    }
};

} // namespace GNC
