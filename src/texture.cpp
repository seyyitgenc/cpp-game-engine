#include <headers/texture.h>

Texture::Texture() {}
Texture::~Texture() {}

// this function will initalize texture and return the new texture
void Texture::initTexture(std::string path, SDL_Renderer *renderer)
{
    // load image into loadedSurface object
    SDL_Surface *loadedSurface = IMG_Load(path.c_str());

    if (loadedSurface == NULL)
        std::cout << "Unable to load image!\n"
                  << SDL_GetError() << std::endl;
    else
    {
        // create texture from surface pixels
        m_newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (m_newTexture == NULL)
            std::cout << "Unable to create texture!\n"
                      << SDL_GetError() << std::endl;
        // get rid of loaded surface
        SDL_FreeSurface(loadedSurface);
    }
}