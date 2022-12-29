#include "Engine/texture.h"

Texture::Texture() {
    this->rTarget = gRenderer;
    m_texture = NULL;
    m_width = 0;
    m_height = 0;
    m_scalex = 1;
    m_scaley = 1;
}
Texture::~Texture() { free(); }
bool Texture::loadFromFile(std::string path) {
    // get rid of preexisting texture
    free();

    SDL_Surface *loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(),
               IMG_GetError());
    } else {
        // TODO enable this. this function ignore given color in texture
        // SDL_SetColorKey(loadedSurface, SDL_TRUE,
        // SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));
        m_texture = IMG_LoadTexture(rTarget, path.c_str());
        m_texture = SDL_CreateTextureFromSurface(rTarget, loadedSurface);
        if (m_texture == NULL) {
            printf("Unable to create texture from %s! SDL Error: %s\n",
                   path.c_str(), SDL_GetError());
        } else {
            m_width = loadedSurface->w;
            m_height = loadedSurface->h;
        }
        SDL_FreeSurface(loadedSurface);
    }
    //  m_texture = IMG_LoadTexture(rTarget, path.c_str());
    // SDL_QueryTexture(m_texture, NULL, NULL, &m_width, &m_height);
    // if (m_texture == NULL)
    // {
    //     printf("Unable to create texture from %s! SDL Error: %s\n",
    //     path.c_str(), SDL_GetError());
    // }
    // Return success
    return m_texture != NULL;
}
int Texture::getWidth() { return m_width; }
void Texture::setWidth(int width) { m_width = width; };
int Texture::getHeight() { return m_height; }
void Texture::setHeight(int height) { this->m_height = height; };
// set texture scale
void Texture::setScale(float scalex, float scaley) {
    m_scalex = scalex;
    m_scaley = scaley;
}
// set image alpha
void Texture::setAlpha(Uint8 alpha) {
    SDL_SetTextureAlphaMod(m_texture, alpha);
}
// set image color
void Texture::setColor(Uint8 red, Uint8 green, Uint8 blue) {
    SDL_SetTextureColorMod(m_texture, red, green, blue);
}
// set image blendmode
void Texture::setBlendMode(SDL_BlendMode blending) {
    SDL_SetTextureBlendMode(m_texture, blending);
}

// render image
void Texture::render(float x, float y, SDL_Rect *clip, double angle,
                     SDL_FPoint *center, SDL_RendererFlip flip) {
    SDL_FRect renderQuad = {x, y, m_width * m_scalex, m_height * m_scaley};

    if (clip != NULL) {
        renderQuad.w = clip->w * m_scalex;
        renderQuad.h = clip->h * m_scaley;
    }
    // draw
    SDL_RenderCopyExF(rTarget, m_texture, clip, &renderQuad, angle, center,
                      flip);
}
// free image
void Texture::free() {
    if (m_texture != NULL) {
        SDL_DestroyTexture(m_texture);
        m_texture = NULL;
        m_width = 0;
        m_height = 0;
    }
}