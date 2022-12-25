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
    static TileManager &get(); // tilemanager singleton
    bool loadTiles(std::string path, float scalex, float scaley);

private:
    
    ~TileManager() = default;
    TileManager() = default;
    static TileManager *s_instance;
};
