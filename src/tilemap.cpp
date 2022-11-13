#include "Headers/tilemap.h"
// FIXME: this tile map is gonna be way bigger find better solution
int map[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

TileMap::TileMap()
{
}

TileMap::~TileMap()
{
}
void TileMap::loadTileMap(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    for (auto tiles : tileMap)
    {
        SDL_RenderDrawRect(renderer, &tiles);
    }
}
void TileMap::initTileMap(const int &tileWidth, const int &tileHeight, const int &screenWidth, const int &screenHeight)
{
    for (int i = 0; i < screenWidth / tileWidth; i++)
    {
        for (int j = 0; j < screenHeight / tileHeight; j++)
        {
            if (map[i + j * (screenWidth / tileWidth)] == 1)
            {
                SDL_Rect tile = {i * tileWidth, j * tileHeight, tileWidth, tileHeight};
                tileMap.push_back(tile);
            }
        }
    }
}