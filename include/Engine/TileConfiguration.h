#pragma once
#include <iostream>
#include "Texture.h"

enum TileType
{
    TILE_AIR,
    TILE_DIRT,
    TILE_GRASS,
    TILE_STONE,
    // TODO: ADD MORE TILES HERE
    TILE_NONE = -1
};

class TileConfiguration
{
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
    SDL_Rect tileClips[4];

private:
    static TileConfiguration *s_instance;
};