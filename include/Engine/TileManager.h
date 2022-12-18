#pragma once
#include "Texture.h"
#include "ECS/Components/Tile.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include "Global.h"

class TileManager
{
public:
    TileManager() = default;   // this can be default
    ~TileManager() = default;  // thic can be default
    static TileManager &get(); // tilemanager singleton
    bool loadTiles();

private:
    static TileManager *s_instance;
};
