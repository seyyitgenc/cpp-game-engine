#pragma once
#include <map>
#include <string>
#include "../SDL2/SDL.h"
#include "../SDL2/SDL_ttf.h"
#include "../SDL2/SDL_image.h"
#include "../Engine/Engine.h"

class AssetManager
{
public:
    AssetManager();
    ~AssetManager() = default;

    void clean();

    SDL_Texture *getTexture(std::string id);
    void loadTexture(std::string id, std::string path);

    TTF_Font *getFont(std::string id);
    void loadFont(std::string id, std::string path, int fontSize);

    void loadRenderedText(std::string id, std::string text);
    void changeText(std::string id, std::string text);

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
    std::map<std::string, TTF_Font *> fonts;
    std::map<std::string, SDL_Texture *> textures;
    SDL_Color textColor = {150, 150, 255, 255};
};