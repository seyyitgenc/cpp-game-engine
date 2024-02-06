#pragma once
#include "glad/glad.h"
#include <stb_image.h>
#include <string>

// todo: refactor this
// general usage utility functions defined here
// Loads texture from specified path
inline bool loadTextureFromFile(const char* path, GLuint *out_texture, int *out_width, int *out_height){
    std::string filename = std::string(path);
    unsigned int textureID;
    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(),  &width, &height, &nrComponents, 0);
    if (data == NULL){
        CLog::write(CLog::Fatal, "Texture failed to load at path : %s\n",path);
        stbi_image_free(data);
        return false;
    }
    GLenum format;
    if (nrComponents == 1)
        format = GL_RED;
    else if(nrComponents == 3)
        format = GL_RGB;
    else if(nrComponents == 4)
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
    *out_texture = textureID;
    *out_width = width;
    *out_height = height;

    return true;
}