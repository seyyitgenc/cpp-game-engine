#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include "globals.h"
#include "texture.h"

class TileManager
{
public:
    TileManager() = default;   // this can be default
    ~TileManager() = default;  // thic can be default
    static TileManager &get(); // tilemanager singleton
    bool loadTiles(std::string path, float scalex, float scaley);

private:
    static TileManager *s_instance;
};
