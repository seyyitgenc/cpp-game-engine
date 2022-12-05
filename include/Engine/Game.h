#pragma once
#include "Engine.h"
#include "headers/tilemap.h"
#include <vector>
#include <string>
#include <iostream>



class Game
{
public:
	Game();
	~Game();
	void Run(); // run main loop

	float &getDeltaTime();

	// Frame Rate Field
	void setFrameRate(const int &SCREEN_FPS);
	void limitFrameRate();
	void initEntities();

private:
	int SCREEN_FPS = 70;
	int SCREEN_TICKS_PER_FRAME = 1000.0f / SCREEN_FPS;
	Engine *engine;

	Clock m_capTimer;
};