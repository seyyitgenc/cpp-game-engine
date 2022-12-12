#include "Engine/Tile.hpp"
#include <fstream>
#include "ECS/AssetManager.h"
SDL_Rect tileClips[4];

Tile::Tile(int x, int y, int tileType)
{
    tileRect.x = x;
    tileRect.y = y;

    tileRect.w = TILE_WIDTH;
    tileRect.h = TILE_HEIGHT;
    // m_texture = AssetManager::get().getTexture("tilemap");
    mType = tileType;
}

// void Tile::render(SDL_FRect&camera)
// {
    
// }