#include <headers/texture.h>

Texture::Texture() {}
Texture::~Texture() {}
Sprite::Sprite() {}
Sprite::~Sprite() {}
TextTexture::TextTexture() {}
TextTexture::~TextTexture() {}
// frees texture used by sprite class
void Sprite::freeTexture()
{
    if (m_texture != NULL)
    {
        SDL_DestroyTexture(m_texture);
        m_texture = NULL;
        m_width = 0;
        m_height = 0;
    }
}
void TextTexture::freeTexture()
{
    if (m_texture != NULL)
    {
        SDL_DestroyTexture(m_texture);
        m_texture = NULL;
        m_width = 0;
        m_height = 0;
    }
}
// initialize texture into the created sprite
bool Sprite::initTexture(std::string path, SDL_Renderer *renderer)
{
    // load image into loadedSurface object
    SDL_Surface *loadedSurface = IMG_Load(path.c_str());

    if (loadedSurface == NULL)
    {
        std::cout << "Unable to load image!\n"
                  << SDL_GetError() << std::endl;
        return false;
    }
    else
    {
        // create texture from surface pixels
        m_texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (m_texture == NULL)
            std::cout << "Unable to create texture!\n"
                      << SDL_GetError() << std::endl;
        // get rid of loaded surface
        SDL_FreeSurface(loadedSurface);
    }
    return true;
}
void TextTexture::closeFont()
{
    TTF_CloseFont(m_font);
    m_font = NULL;
}

bool TextTexture::loadRenderedText(std::string textureText, SDL_Color textColor, SDL_Renderer *renderer)
{
    freeTexture();
    m_font = TTF_OpenFont("fonts/lazy.ttf", 28);
    if (m_font == NULL)
    {
        std::cout << "Failed to load lazy font!\n"
                  << TTF_GetError() << std::endl;
        return false;
    }
    else
    {
        SDL_Surface *textSurface = TTF_RenderText_Solid(m_font, textureText.c_str(), textColor);
        if (textSurface == NULL)
        {
            std::cout << "Unable to render text surface!\n"
                      << TTF_GetError() << std::endl;
            return false;
        }
        else
        {
            m_texture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (m_texture == NULL)
            {
                std::cout << "Unable to create texture fom rendered text!\n"
                          << SDL_GetError() << std::endl;
                return false;
            }
            else
            {
                m_width = textSurface->w;
                m_height = textSurface->h;
            }
            SDL_FreeSurface(textSurface);
        }
    }
    return true;
}