#include <headers/texture.h>

Sprite::Sprite() {}
Sprite::~Sprite() {}
TextToTexture::TextToTexture() {}
TextToTexture::~TextToTexture() {}

// free texture used by  Texture derivied class object
void Texture::freeTexture()
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
// this function use hardware rendering
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
// it can be usefull later when rendering multiple textures
SDL_Surface *Sprite::loadSurface(std::string path, SDL_Surface *screenSurface)
{
    // the final optimized image
    SDL_Surface *optimizedSurface = NULL;
    SDL_Surface *loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL)
    {
        std::cout << "Unable to load image\n"
                  << SDL_GetError() << std::endl;
    }
    else
    {
        // convert surface to screen format
        optimizedSurface = SDL_ConvertSurface(loadedSurface, screenSurface->format, 0);
        if (optimizedSurface == NULL)
            std::cout << "Unable to optimize image" << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
    }
    return optimizedSurface;
}
// close font used by TextToTexture class
void TextToTexture::closeFont()
{
    if (m_font != NULL)
    {
        m_font = NULL;
        TTF_CloseFont(m_font);
        std::cout<<"font closed"<<std::endl;
    }
}
// give font path
bool TextToTexture::setFont(const std::string path)
{
    m_font = TTF_OpenFont(path.c_str(), 22);
    m_path = path;
    if (m_font == NULL)
    {
        std::cout << "Unable to load font!" << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}
TTF_Font *TextToTexture::m_font{NULL};
std::string TextToTexture::m_path{"fonts/aerial.ttf"};
// Load specified text to texture
bool TextToTexture::loadRenderedText(std::string textureText, SDL_Color textColor, SDL_Renderer *renderer)
{
    freeTexture();
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
            setSize({textSurface->w, textSurface->h});
        }
        SDL_FreeSurface(textSurface);
    }
    return true;
}
