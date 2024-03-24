#pragma once

#include "../util/log.hpp"
#include "../external/stb_image.h"
#include "../external/glad/glad.h"

#include <string>
#include <memory>
#include <unordered_map>

enum TextureType{NONE = 0, ICON, DIFFUSE, SPECULAR, NORMAL, DEPTH, HEIGHT};

struct Texture
{
    // load texture with given path and specify it's type
    [[nodiscard]] bool loadTexture(const std::string &path, TextureType type){
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data == NULL){
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::LOAD_TEXTURE Failed to load texture with path -> "),
                YELLOW_TEXT(path),
                "\n");
            stbi_image_free(data);
            return false;
        }
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        Log::write(
            Log::Debug,
            LIGHT_MAGENTA_TEXT("DEBUG::LOAD_TEXTURE Texture loaded. name -> "),
            YELLOW_TEXT(file_name),
            LIGHT_MAGENTA_TEXT(", ID -> "),
            ID,
            "\n");
        return true;
    }
    
    TextureType type = NONE;
    std::string file_name = "";
    GLuint      ID = -1;
    int width = -1;
    int height = -1;
    int nrComponents = -1;
};

// Texture manager to prevent loading same image again and again
class TextureManager
{
public:
    TextureManager(TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;

    static TextureManager *getInstance(){
        if (_instance == nullptr)
        {
            _instance = new TextureManager();
            Log::write(
                Log::Info,
                GREEN_TEXT("INFO::TEXTURE_MANAGER::GET_INSTANCE Texture manager initalized succesfully with mem address -> "),
                &_instance,
                "\n");
        }
        return _instance;
    }

    // adds texture with given name and type. returns if texture exist
    void addTexture(const std::string &name, const std::string &path, TextureType type){
        if(isTextureExist(name)) // early rejection. if texture exist then returns
            return;
        // todo: handle null returned texture object
        auto texture = std::make_unique<Texture>();
        texture->file_name = name;
        if(!texture->loadTexture(path, type))
            return;
        _textures[name] = std::move(texture);
    }
    // void removeTexture(const std::string &name){}

    // returns texture pointer to given texture name. returns nullptr if texture is not exist
    [[nodiscard]] Texture* getTexture(const std::string &name){
        if(!isTextureExist(name)){
            Log::write(
                    Log::Fatal,
                    LIGHT_RED_TEXT("FATAL::TEXTURE_MANAGER::GET_TEXTURE Your tried to get texture that doesn't exist. Name you provided is -> "),
                    YELLOW_TEXT(name),
                    LIGHT_RED_TEXT(" due to that function returned nullptr. Application may crash.\n"));
            return nullptr;
        }
        return _textures[name].get();
    }

    // returns texture id from given texture name. returns -1 if texture is not exist
    [[nodiscard]] int getTextureId(const std::string &name){
        if(!isTextureExist(name)){
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::TEXTURE_MANAGER::GET_TEXTUREID You tried to get textureID that doesn't exists. Name you provided is -> "),
                YELLOW_TEXT(name),
                LIGHT_RED_TEXT(" due to that function returning -1.\n"));
            return -1;
        }
        return _textures[name]->ID;
    }

private:
    TextureManager() = default;
    // checks if texture exist in _textures map
    [[nodiscard]] bool isTextureExist(const std::string &name){
        return _textures.find(name) != _textures.end();
    }
    static TextureManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
};

inline TextureManager* TextureManager:: _instance = nullptr;