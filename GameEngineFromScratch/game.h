#pragma once
#include "SDL.h"
#include <iostream>
class Game
{
public:
	Game();
	~Game();
	void Init(const char*, int, int, int, int, int);
	void handleEvents();
	void render();
	void update();
	void setFullScreen();

	bool isRunnig() { return isOpen; }
private:
	bool isOpen = false;
	bool fullscreen = false;
	SDL_Renderer* renderer;
	SDL_Window* window;
	int flags = 0;
};