#include "Engine/TileManager.h"
#include <fstream>

TileManager *TileManager::s_instance;

TileManager &TileManager::get()
{
    if (s_instance == nullptr)
    {
        std::cout << "Tile Manager Initialized " << std::endl;
        s_instance = new TileManager();
    }
    return *s_instance;
}

bool TileManager::setTiles()
{
    bool tilesLoaded = true;
    int x = 0, y = 0;
    std::fstream map("./resources/lazy.map");
    if (map.fail())
    {
        std::cout << "Unable to load map file" << std::endl;
        tilesLoaded = false;
    }
    else
    {
        for (int i = 0; i < TOTAL_TILES; i++)
        {
            int tileType = -1;
            map >> tileType;
            if (map.fail())
            {
                // Stop loading map
                printf("Error loading map: Unexpected end of file!\n");
                tilesLoaded = false;
                break;
            }
            if ((tileType >= 0) && (tileType < 12))
            {
                auto &entity = Manager::get().addEntity();
                entity.addComponent<Transform>(0, 0, TileConfiguration::get().getScale().x, TileConfiguration::get().getScale().y, 0);
                entity.addComponent<Tile>(x, y, tileType);
            }
            else
            {
                // Stop loading map
                printf("Error loading map: Invalid tile type at %d!\n", i);
                tilesLoaded = false;
                break;
            }
            x += TILE_WIDTH * TileConfiguration::get().getScale().x;
            if (x >= LEVEL_WIDTH)
            {
                x = 0;
                y += TILE_HEIGHT * TileConfiguration::get().getScale().y;
            }
        }
        if (tilesLoaded)
        {
            TileConfiguration::get().initTileClips();
        }
    }
    map.close();
    return tilesLoaded;
}