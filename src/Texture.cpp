#include "Engine/Texture.h"

Texture::Texture(SDL_Renderer *rTarget)
{
    this->rTarget = rTarget;
    m_texture = NULL;
    m_width = 0;
    m_height = 0;
}
Texture::~Texture()
{
    free();
}
bool Texture::loadFromFile(std::string path)
{
    // get rid of preexisting texture
    free();

    SDL_Surface *loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
    }
    else
    {
        // TODO enable this. this function ignore given color in texture
        // SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));
        m_texture = IMG_LoadTexture(rTarget, path.c_str());
        m_texture = SDL_CreateTextureFromSurface(rTarget, loadedSurface);
        if (m_texture == NULL)
        {
            printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
        }
        else
        {
            m_width = loadedSurface->w;
            m_height = loadedSurface->h;
        }
        SDL_FreeSurface(loadedSurface);
    }
    //  m_texture = IMG_LoadTexture(rTarget, path.c_str());
    // SDL_QueryTexture(m_texture, NULL, NULL, &m_width, &m_height);
    // if (m_texture == NULL)
    // {
    //     printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
    // }
    // Return success
    return m_texture != NULL;
}
int Texture::getWidth() { return m_width; }
void Texture::setWidth(int width) { this->m_width = width; };
int Texture::getHeight() { return m_height; }
void Texture::setHeight(int height) { this->m_height = height; };
// set image alpha
void Texture::setAlpha(Uint8 alpha) { SDL_SetTextureAlphaMod(m_texture, alpha); }
// set image color
void Texture::setColor(Uint8 red, Uint8 green, Uint8 blue) { SDL_SetTextureColorMod(m_texture, red, green, blue); }
// set image blendmode
void Texture::setBlendMode(SDL_BlendMode blending) { SDL_SetTextureBlendMode(m_texture, blending); }

// render image
void Texture::render(int x, int y, SDL_Rect *clip, double angle, SDL_Point *center, SDL_RendererFlip flip)
{
    SDL_Rect renderQuad = {x, y, m_width, m_height};

    if (clip != NULL)
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    // draw
    SDL_RenderCopyEx(rTarget, m_texture, clip, &renderQuad, angle, center, flip);
}
// free image
void Texture::free()
{
    if (m_texture != NULL)
    {
        SDL_DestroyTexture(m_texture);
        m_texture = NULL;
        m_width = 0;
        m_height = 0;
    }
}