#pragma once
#ifndef GAME_H
#define GAME_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <headers/timer.h>
#include <headers/window.h>
#include <headers/texture.h>
#endif

class Game
{
public:
	Game();
	~Game();
	void Run();									// run main loop
	void setScreenFps(int, bool);				// set screen fps
	void handleEvents();						// handles events created by user
	void handleKeyStates(const std::uint8_t *); // handles key events this will be used with player movement
	void Update(float dt);						// updates game physics
	int getFrameRate(int, Uint32);				// get frame rate
	void limitFrameRate();						// limits the fps to the fps limit

private:
	// window and renderer field
	Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
	Window window;

	// Game Tick and FPS field
	int SCREEN_FPS = 60;
	int SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
	bool m_isFrameLimitEnabled = false;
	// Timer field
	Timer m_fpsTimer;
	Timer m_capTimer;
	unsigned int m_countedFrames = 0;
};