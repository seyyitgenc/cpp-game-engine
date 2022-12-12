#pragma once
#include "Global.h"
#include <fstream>
static const int TILE_WIDTH = 32;
static const int TILE_HEIGHT = 32;

enum TileType
{
    TILE_AIR,
    TILE_DIRT,
    TILE_GRASS,
    TILE_STONE,
    // TODO: ADD MORE TILES HERE
    TILE_NONE = -1
};

class Tile
{
public:
    Tile(int x, int y, int tileType);
    // void render(SDL_FRect& camera);
    // bool setTiles(Tile *tiles[]);

private:
    SDL_Texture *m_texture;
    SDL_Rect tileRect;
    int mType;
};

static bool setTiles(Tile *tiles[])
{
    bool tilesLoaded = true;
    int x = 0, y = 0;
    std::ifstream map("resources/lazy.map");
    if (map.fail())
    {
        std::cout << "Unable to load map file" << std::endl;
        tilesLoaded = false;
    }
    else
    {
        // TODO change this 192 to TOTAL_TILES
        for (int i = 0; i < 192; i++)
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

            // if the number is a valid tile number
            // TODO change this 12 to total tile sprites
            if ((tileType >= 0) && (tileType < 12))
            {
                tiles[i] = new Tile(x, y, tileType);
            }
            else
            {
                // Stop loading map
                printf("Error loading map: Invalid tile type at %d!\n", i);
                tilesLoaded = false;
                break;
            }
            x += TILE_WIDTH;
            // If we've gone too far
            if (x >= LEVEL_WIDTH)
            {
                // Move back
                x = 0;
                // Move to the next row
                y += TILE_HEIGHT;
            }
        }
        if (tilesLoaded)
        {
            // tileClips[TILE_AIR].x = 0;
            // tileClips[TILE_AIR].y = 0;
            // tileClips[TILE_AIR].w = TILE_WIDTH;
            // tileClips[TILE_AIR].h = TILE_HEIGHT;

            // tileClips[TILE_DIRT].x = 32;
            // tileClips[TILE_DIRT].y = 0;
            // tileClips[TILE_DIRT].w = TILE_WIDTH;
            // tileClips[TILE_DIRT].h = TILE_HEIGHT;

            // tileClips[TILE_GRASS].x = 64;
            // tileClips[TILE_GRASS].y = 0;
            // tileClips[TILE_GRASS].w = TILE_WIDTH;
            // tileClips[TILE_GRASS].h = TILE_HEIGHT;

            // tileClips[TILE_STONE].x = 96;
            // tileClips[TILE_STONE].y = 0;
            // tileClips[TILE_STONE].w = TILE_WIDTH;
            // tileClips[TILE_STONE].h = TILE_HEIGHT;
        }
    }
    map.close();
    return tilesLoaded;
}
