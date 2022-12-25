#include "Engine/tileconfiguration.h"

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

// Initialize tile clips here
void TileConfiguration::initTileClips()
{
    // clang-format off
    //----------------------------------GRASS TILES---------------------------------------//
    // Grass tile 1                                 //Grass tile 2
    tileClips[TILE_GRASS1].x = 0;                   tileClips[TILE_GRASS2].x = 32;
    tileClips[TILE_GRASS1].y = 0;                   tileClips[TILE_GRASS2].y = 0;
    tileClips[TILE_GRASS1].w = TILE_WIDTH;          tileClips[TILE_GRASS2].w = TILE_WIDTH;
    tileClips[TILE_GRASS1].h = TILE_HEIGHT;         tileClips[TILE_GRASS2].h = TILE_HEIGHT;
    
    // Grass tile 3                                 // Grass tile 4
    tileClips[TILE_GRASS3].x = 0;                   tileClips[TILE_GRASS4].x = 32;
    tileClips[TILE_GRASS3].y = 32;                  tileClips[TILE_GRASS4].y = 32;
    tileClips[TILE_GRASS3].w = TILE_WIDTH;          tileClips[TILE_GRASS4].w = TILE_WIDTH;
    tileClips[TILE_GRASS3].h = TILE_HEIGHT;         tileClips[TILE_GRASS4].h = TILE_HEIGHT;
    
    //----------------------------------PATH TILES----------------------------------------//
    // Path tile 1                                  // Path tile 2
    tileClips[TILE_PATH1].x = 0;                    tileClips[TILE_PATH2].x = 32;
    tileClips[TILE_PATH1].y = 128;                  tileClips[TILE_PATH2].y = 128;
    tileClips[TILE_PATH1].w = TILE_WIDTH;           tileClips[TILE_PATH2].w = TILE_WIDTH;
    tileClips[TILE_PATH1].h = TILE_HEIGHT;          tileClips[TILE_PATH2].h = TILE_HEIGHT;
    // Path tile 3                                  // Path tile 4
    tileClips[TILE_PATH3].x = 0;                    tileClips[TILE_PATH4].x = 32;
    tileClips[TILE_PATH3].y = 160;                  tileClips[TILE_PATH4].y = 160;
    tileClips[TILE_PATH3].w = TILE_WIDTH;           tileClips[TILE_PATH4].w = TILE_WIDTH;
    tileClips[TILE_PATH3].h = TILE_HEIGHT;          tileClips[TILE_PATH4].h = TILE_HEIGHT;

    //----------------------------------FLAWOR TILES-------------------------------------//

    // Flawor tile 1                                // Flawor tile 2
    tileClips[TILE_FLAWOR1].x = 128;                tileClips[TILE_FLAWOR2].x = 160;
    tileClips[TILE_FLAWOR1].y = 0;                  tileClips[TILE_FLAWOR2].y = 0;
    tileClips[TILE_FLAWOR1].w = TILE_WIDTH;         tileClips[TILE_FLAWOR2].w = TILE_WIDTH;
    tileClips[TILE_FLAWOR1].h = TILE_HEIGHT;        tileClips[TILE_FLAWOR2].h = TILE_HEIGHT;
    
    // Flawor tile 3                                // Flawor tile 4
    tileClips[TILE_FLAWOR3].x = 128;                tileClips[TILE_FLAWOR4].x = 160;
    tileClips[TILE_FLAWOR3].y = 32;                 tileClips[TILE_FLAWOR4].y = 32;
    tileClips[TILE_FLAWOR3].w = TILE_WIDTH;         tileClips[TILE_FLAWOR4].w = TILE_WIDTH;
    tileClips[TILE_FLAWOR3].h = TILE_HEIGHT;        tileClips[TILE_FLAWOR4].h = TILE_HEIGHT;
    // clang-format on
}