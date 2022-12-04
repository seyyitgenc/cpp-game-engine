#include "ECS/AssetManager.h"
#include <iostream>
AssetManager *AssetManager::s_instance;
AssetManager::AssetManager()
{
    if (TTF_Init() != 0)
        std::cerr << TTF_GetError() << std::endl;
    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) != 0)
        std::cerr << TTF_GetError() << std::endl;
}

SDL_Texture *AssetManager::getTexture(std::string id)
{
    // if texture is exist then return
    return (textures.count(id) > 0) ? textures[id] : nullptr;
}

void AssetManager::loadTexture(std::string id, std::string path)
{
    if (textures.count(id) <= 0) // if same texture is exist don't create new texture
    {
        SDL_Texture *texture = IMG_LoadTexture(Engine::get().getRenderer(), path.c_str());
        if (texture)
        {
            textures.emplace(id, texture);
            std::cout << "Texture :[" << path << "] loaded" << std::endl;
        }
        else
        {
            std::cerr << IMG_GetError() << std::endl;
        }
    }
}
void AssetManager::loadFont(std::string id, std::string path, int fontSize)
{
    TTF_Font *newFont = TTF_OpenFont(path.c_str(), fontSize);
    if (newFont != nullptr)
    {
        fonts.emplace(id, newFont); // this section ensures that font saved only once
        std::cout << "Font :[" << path << "] loaded" << std::endl;
    }
    else
        std::cerr << TTF_GetError() << std::endl;
}
TTF_Font *AssetManager::getFont(std::string id)
{
    if (fonts.count(id) > 0)
        return fonts[id];
    return nullptr;
}

// this cleans everything related to Textures and Fonts
void AssetManager::clean()
{
    for (auto it = textures.begin(); it != textures.end(); it++)
    {
        SDL_DestroyTexture(it->second);
        // FIXME this line of code is bugged
        // textures.erase(it); 
    }
    textures.clear();
    std::cout << "Textures Cleared !" << std::endl;

    for (auto it = fonts.begin(); it != fonts.end(); it++)
    {
        TTF_CloseFont(it->second);
        fonts.erase(it);
    }
    fonts.clear();
    std::cout << "Fonts Cleared !" << std::endl;
}