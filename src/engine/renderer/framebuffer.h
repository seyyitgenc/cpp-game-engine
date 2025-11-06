#pragma once

#include "../globals.h"

/*
Color,
depth,
stencil
depth-stencil
*/

typedef enum struct FrameBufferTextureType {
    NONE = 0,
    POSITION,
    NORMAL,
    ALBEDO,
    SPECULAR,
    DISPLACEMENT,
    SHADOW,
    ROUGHNESS,
} FBTT;

class FrameBufferTexture {
public:
    FrameBufferTexture(int width, int height, FBTT type, GLenum attachment)
        : _width(width)
        , _height(height)
        , _type(type)
        , _attachment(attachment)
    {
        Configure();
    }

    void cleanup()
    {
        if (_texture != 0 && glIsTexture(_texture)) {
            glDeleteTextures(1, &_texture);
            std::cout << "deleted" << std::endl;
        }
    }

    void resize(int width, int height)
    {
        _width = width;
        _height = height;
        Configure();
    }

    void Configure();

    int _width = -1;
    int _height = -1;
    FBTT _type = FBTT::NONE;
    GLenum _attachment = GL_NONE;
    GLuint _texture = 0;
};

struct FrameBuffer {
    FrameBuffer();
    ~FrameBuffer();
    void bind(GLenum target);
    void attachTexture(int width, int height, FBTT type, GLenum attachment);
    void bindTextures();
    void attachRenderBuffer();
    void checkCompleteness();
    void unbind();
    void resizeBuffer(int width, int height);

    std::vector<FrameBufferTexture> _boundTextures;
    int attachementsCount;
    GLuint _framebuffer;
    GLuint _renderbuffer;
};
