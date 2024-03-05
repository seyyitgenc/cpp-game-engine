#pragma once

#include "util/log.hpp"

#include "glad/glad.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <stb_image.h>

enum TextureType{NONE, ICON, DIFFUSE, SPECULAR, NORMAL, DEPTH, HEIGHT};

struct Texture
{  
    TextureType type;
    std::string path;
    unsigned int id;
    // int width;
    // int height;
};
Texture loadTextureFromFile(const std::string &path ,TextureType type);

// Texture manager for texture usage that not used by models. for example: imgui::image
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
                GREEN_TEXT("INFO::TEXTURE_MANAGER::GET_INSTANCE "), 
                YELLOW_TEXT("Texture manager initalized succesfully with mem address -> '"),
                &_instance, YELLOW_TEXT("'\n"));
        }
        return _instance;
    }
    void addTexture(const std::string &name, const std::string &path, TextureType type){
        if(isTextureExist(name)){
            Log::write(
                Log::Warning, 
                LIGHT_YELLOW_TEXT("WARNING::TEXTURE_MANAGER::ADD_TEXTURE"), 
                YELLOW_TEXT(" You tried to load texture that already exists with the name of : '"), 
                YELLOW_TEXT(name), YELLOW_TEXT("' please check your texture name\n"));
            return;
        }
        // todo: handle null returned texture object
        _textures[name] = std::make_unique<Texture>(loadTextureFromFile(path, type));
    }
    // void removeTexture(const std::string &name){}
    [[nodiscard]] Texture* getTexture(const std::string &name){
        if(!isTextureExist(name)){
            Log::write(
                    Log::Fatal,
                    LIGHT_RED_TEXT("FATAL::TEXTURE_MANAGER::GET_TEXTURE "),
                    YELLOW_TEXT("Your tried to get texture that doesn't exist. Name you provided is : '"), 
                    YELLOW_TEXT(name), 
                    YELLOW_TEXT("' due to that function returned nullptr. Application may crash.\n"));
            return nullptr;
        }
        return _textures[name].get();
    }
    [[nodiscard]] int getTextureId(const std::string &name){
        if(!isTextureExist(name)){
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::TEXTURE_MANAGER::GET_TEXTURE_ID "), 
                YELLOW_TEXT("Your tried to get texture_id that doesn't exists. Name you provided is : '"),
                YELLOW_TEXT(name), 
                YELLOW_TEXT("' due to that function returned -1.\n"));
            return -1;
        }
        return getTexture(name)->id;
    }

private:
    TextureManager() = default;
    [[nodiscard]] bool isTextureExist(const std::string &name){
        return _textures.find(name) != _textures.end();
    }
    static TextureManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
};

inline TextureManager* TextureManager:: _instance = nullptr;

inline Texture loadTextureFromFile(const std::string &path ,TextureType type){
    unsigned int textureID;
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data == NULL)
    {
        Log::write(
            Log::Fatal, 
            LIGHT_RED_TEXT("FATAL::LOAD_TEXTURE_FROM_FILE "), 
            YELLOW_TEXT("Texture failed to load at path : '"), YELLOW_TEXT(path), YELLOW_TEXT("'\n"));
        stbi_image_free(data);
        return {NONE, path, 1};
        // return nullptr;
        // todo: don't forget to handle this nullptr case
    }
    GLenum format;
    if (nrComponents == 1)
        format = GL_RED;
    else if (nrComponents == 3)
        format = GL_RGB;
    else if (nrComponents == 4)
        format = GL_RGBA;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    Log::write(
        Log::Debug, 
        LIGHT_MAGENTA_TEXT("DEBUG::LOAD_TEXTURE_FROM_FILE ") ,
        YELLOW_TEXT("Loaded texture from file with path -> "), 
        YELLOW_TEXT(path), YELLOW_TEXT(" and with ID : '"), textureID, YELLOW_TEXT("'\n"));
    
    return Texture{type, path, textureID/*, width, height*/};
}