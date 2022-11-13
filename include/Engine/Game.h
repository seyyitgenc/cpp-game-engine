#pragma once
#include "Engine.h"
#include "../headers/tilemap.h"
#include <vector>
#include <string>
#include <iostream>

class Game
{
public:
	Game();
	~Game();
	void Run();					   // run main loop
	void setScreenFps(int, bool);  // set screen fps
	void Update(float dt);		   // updates game physics
	int getFrameRate(int, Uint32); // get frame rate
	void limitFrameRate();		   // limits the fps to the fps limit
public:
	Clock timer;

private:
	Engine engine;
	// Tilemap Filter
	TileMap map;
};