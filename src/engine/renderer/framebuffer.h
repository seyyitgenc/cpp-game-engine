#pragma once

#include "../globals.h"

typedef enum struct FrameBufferTextureType{
    NONE = 0,
    POSITION,
    NORMAL,
    ALBEDO,
    SPECULAR,
    DISPLACEMENT,
    SHADOW,
    ROUGHNESS,
} FBTT;

struct FrameBufferTexture{
    // TODO : take these args as pointer to prevent extre copying
    FrameBufferTexture(int width, int height, FBTT type,GLenum attachment) 
    : _width(width), _height(height), _type(type), _attachment(attachment) {}
    void Configure();
    int _width = -1;
    int _height = -1;
    GLenum _attachment = GL_NONE;
    FBTT _type = FBTT::NONE;
    GLuint _texture = -1;
};

struct FrameBuffer{
    FrameBuffer();
    void bind();
    void attachTexture(int width, int height, FBTT type,GLenum attachment);
    void bindTextures();
    void attachRenderBuffer();
    bool checkCompleteness();
    void unbind();

    std::vector<FrameBufferTexture> _boundTextures;
    int attachementsCount;
    GLuint _framebuffer;
    GLuint _renderbuffer;
};
