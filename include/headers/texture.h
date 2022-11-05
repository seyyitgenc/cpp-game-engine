#ifndef TEXTURE_H
#define TEXTURE_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#endif

class Texture
{
public:
    Texture();
    ~Texture();
    void initTexture(std::string path, SDL_Renderer *renderer);
    SDL_Texture *getTexture() { return m_texture; }
    SDL_Texture *getNewTexture() { return m_newTexture; }

private:
    SDL_Texture *m_texture;
    SDL_Texture *m_newTexture;
};
