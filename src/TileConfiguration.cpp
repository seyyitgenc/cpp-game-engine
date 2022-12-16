#include "Engine/TileConfiguration.h"

TileConfiguration *TileConfiguration::s_instance;

TileConfiguration::TileConfiguration()
{
    m_scalex = 1;
    m_scaley = 1;
}
// TileConfiguration singleton
TileConfiguration &TileConfiguration::get()
{
    if (s_instance == nullptr)
    {
        std::cout << "Tile Texture Initialized" << std::endl;
        s_instance = new TileConfiguration();
    }
    return *s_instance;
}
void TileConfiguration::configure(std::string path, float scalex, float scaley)
{
    tileTexture = new Texture();
    tileTexture->loadFromFile(path);
    tileTexture->setScale(scalex, scaley);
    m_scalex = scalex;
    m_scaley = scaley;
}
// returns scale
vf2d TileConfiguration::getScale() { return vf2d(m_scalex, m_scaley); }

// Initialize tileclipshere
void TileConfiguration::initTileClips()
{
    // air tile
    tileClips[TILE_AIR].x = 0;
    tileClips[TILE_AIR].y = 0;
    tileClips[TILE_AIR].w = TILE_WIDTH;
    tileClips[TILE_AIR].h = TILE_HEIGHT;

    tileClips[TILE_DIRT].x = 128;
    tileClips[TILE_DIRT].y = 0;
    tileClips[TILE_DIRT].w = TILE_WIDTH;
    tileClips[TILE_DIRT].h = TILE_HEIGHT;

    // tileClips[TILE_GRASS].x = 64;
    // tileClips[TILE_GRASS].y = 0;
    // tileClips[TILE_GRASS].w = 32;
    // tileClips[TILE_GRASS].h = 32;

    // tileClips[TILE_STONE].x = 96;
    // tileClips[TILE_STONE].y = 0;
    // tileClips[TILE_STONE].w = 32;
    // tileClips[TILE_STONE].h = 32;
}