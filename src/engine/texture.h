#pragma once

#include "util/log.h"

#include "glad/glad.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <stb_image.h>

enum TextureType{NONE, ICON, DIFFUSE, SPECULAR, NORMAL, DEPTH};

struct Texture
{  
    TextureType type;
    std::string path;
    unsigned int id;
    int width;
    int height;
    void isOk();
};

inline Texture loadTextureFromFile(const std::string &path ,TextureType type){
    unsigned int textureID;
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data == NULL)
    {
        CLog::write(CLog::Fatal, "FATAL::LOAD_TEXTURE_FROM_FILE Texture failed to load at path : %s\n",path);
        stbi_image_free(data);
        return {};
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
    CLog::write(CLog::Debug, "DEBUG::LOAD_TEXTURE_FROM_FILE Loaded texture from file with path -> %s and with ID : %d\n", path.c_str(), textureID);
    
    return Texture{type, path, textureID, width, height};;
}
class TextureManager
{
public:
    static TextureManager *getInstance(){
        if (_instance == nullptr)
        {
            _instance = new TextureManager();
            CLog::write(CLog::Info, "INFO::TEXTURE_MANAGER::GET_INSTANCE Texture manager initalized succesfully with Mem address of -> %p\n", _instance);
        }
        return _instance;
    }
    void addTexture(const std::string &name, const std::string &path, TextureType type){
        // Texture returnedTexture = loadTextureFromFile(path ,type);
        if(isTextureExist(name)){
            CLog::write(CLog::Warning,"WARNING::TEXTURE_MANAGER::ADD_TEXTURE You tried to load texture that already exists with the name of : '%s' please check your texture name\n", name.c_str());
            return;
        }
        _textures[name] = std::make_unique<Texture>(loadTextureFromFile(path, type));
    }
    // void removeTexture(const std::string &name){}
    Texture &getTexture(const std::string &name){ return *_textures[name];}
    int getTextureId(const std::string &name){
          if(!isTextureExist(name)){
            CLog::write(CLog::Warning,"WARNING::TEXTURE_MANAGER::GET_TEXTURE_ID Your tried to get texture_id that doesn't exists name you provided is : '%s' due to that function returned -1. \n", name.c_str());
            return -1;
        }
        return getTexture(name).id;
    }

private:
    TextureManager() = default;
    bool isTextureExist(const std::string &name){
        return _textures.find(name) != _textures.end();
    }
    static TextureManager * _instance;
    std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
};

inline TextureManager * TextureManager:: _instance = nullptr;
