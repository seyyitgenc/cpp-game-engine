#pragma once
#include <iostream>

#include "texture.h"

enum TileType {
    TILE_GRASS1,
    TILE_GRASS2,
    TILE_GRASS3,
    TILE_GRASS4,
    TILE_PATH1,
    TILE_PATH2,
    TILE_PATH3,
    TILE_PATH4,
    TILE_FLAWOR1,
    TILE_FLAWOR2,
    TILE_FLAWOR3,
    TILE_FLAWOR4,
    // TODO: ADD MORE TILES HERE
    TILE_NONE = -1
};

class TileConfiguration {
   public:
    TileConfiguration();
    ~TileConfiguration() = default;
    static TileConfiguration &get();
    void configure(std::string path, float scalex, float scaley);
    void initTileClips();
    vf2d getScale();

   public:
    float m_scalex, m_scaley;
    Texture *tileTexture;
    SDL_Rect tileClips[TOTAL_TILE_SPRITES];

   private:
    static TileConfiguration *s_instance;
};