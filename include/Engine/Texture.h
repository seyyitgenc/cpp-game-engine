#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include <string>

// Texture wrapper for SDL_Texture
class Texture
{
public:
    Texture(SDL_Renderer *rTarget);
    ~Texture();
    bool loadFromFile(std::string path);
    void free();
    void setColor(Uint8 red, Uint8 green, Uint8 blue);
    void setBlendMode(SDL_BlendMode blending);
    void setAlpha(Uint8 alpha);
    void render(int x, int y, SDL_Rect *clip = NULL, double angle = 0.0, SDL_Point *center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);
    int getWidth();
    int getHeight();
    void setWidth(int width);
    void setHeight(int height);

private:
    SDL_Texture *m_texture = NULL;
    SDL_Renderer *rTarget = NULL;
    int m_width;
    int m_height;
};
