#pragma once
#include <string>
#include "engine.h"
#include <SDL_image.h>

// Texture wrapper for SDL_Texture
class Texture
{
public:
    Texture();
    ~Texture();
    bool loadFromFile(std::string path);
    void free();
    void setColor(Uint8 red, Uint8 green, Uint8 blue);
    void setBlendMode(SDL_BlendMode blending);
    void setScale(float scalex, float scaley);
    void setAlpha(Uint8 alpha);
    void render(float x, float y, SDL_Rect *clip = NULL, double angle = 0.0, SDL_FPoint *center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);
    int getWidth();
    int getHeight();
    void setWidth(int width);
    void setHeight(int height);

private:
    float m_scalex;
    float m_scaley;
    SDL_Texture *m_texture = NULL;
    SDL_Renderer *rTarget = NULL;
    int m_width;
    int m_height;
};
