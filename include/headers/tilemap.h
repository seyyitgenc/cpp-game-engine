#pragma once
#include "Engine/Engine.h"
#include <vector>

class TileMap
{
public:
    TileMap();
    ~TileMap();
    void initTileMap(const int &tileWidth, const int &tileHeight, const int &screenWidth, const int &screenHeight);
    void loadTileMap(SDL_Renderer *renderer);
    void setTileSize(int width, int height)
    {
        m_tileHeight = height;
        m_tileWidth = width;
    };
    void setMapSize(int width, int height)
    {
        m_mapHeight = height;
        m_mapWidth = width;
    }

private:
    std::vector<SDL_Rect> tileMap;
    int m_tileWidth, m_tileHeight;
    int m_mapWidth, m_mapHeight;
};
