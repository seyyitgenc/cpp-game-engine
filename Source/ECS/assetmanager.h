#pragma once
#include <map>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include "Engine/engine.h"
#include "Engine/texture.h"

// TODO name this as a resource manager

class AssetManager
{
public:
    AssetManager();
    ~AssetManager() = default;

    void clean();

    Texture *getTexture(std::string id);
    void loadTexture(std::string id, std::string path);

    TTF_Font *getFont(std::string id);
    void loadFont(std::string id, std::string path, int fontSize);

    static AssetManager &get()
    {
        if (s_instance == nullptr)
        {
            std::cout << "Asset Manager Initialized!" << std::endl;
            s_instance = new AssetManager();
        }
        return *s_instance;
    }

private:
    static AssetManager *s_instance;
    std::map<std::string, Texture *> textures;
    std::map<std::string, TTF_Font *> fonts;
    SDL_Color textColor = {150, 150, 255, 255};
};