#pragma once

#include <memory>
#include <string>
#include <unordered_map>

enum struct TextureType {
    NONE = 0,
    ICON,
    DIFFUSE,
    SPECULAR,
    NORMAL,
    DEPTH,
    HEIGHT,
    REFLECTION,
    DISPLACEMENT,
    ROUGNESS
};

struct Texture {
    // load texture with given path and specify it's type
    [[nodiscard]] bool loadTexture(const std::string& path, TextureType type);

    TextureType type = TextureType::NONE;
    std::string file_name = "";
    unsigned int ID = -1;
    int width = -1;
    int height = -1;
    int nrComponents = -1;
};

// Texture manager to prevent loading same image again and again
class TextureManager {
public:
    TextureManager(TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;

    // return texture manager instance
    static TextureManager* instance();

    // adds texture with given name and type. returns if texture exist
    void addTexture(const std::string& name, const std::string& path, TextureType type);
    // void removeTexture(const std::string &name){}

    // returns texture pointer to given texture name. returns nullptr if texture is not exist
    [[nodiscard]] Texture* getTexture(const std::string& name);

    // returns texture id from given texture name. returns -1 if texture is not exist
    [[nodiscard]] int getTextureId(const std::string& name);

private:
    TextureManager() = default;
    // checks if texture exist in _textures map
    [[nodiscard]] bool isTextureExist(const std::string& name) { return _textures.find(name) != _textures.end(); }
    static TextureManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
};

inline TextureManager* TextureManager::_instance = nullptr;