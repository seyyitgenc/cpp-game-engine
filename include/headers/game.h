#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <headers/timer.h>
#include <headers/window.h>
class Game
{
public:
	Game();
	~Game();
	void Run();

	void setScreenFps(int, bool);

	void handleEvents();
	void handleKeyStates(const std::uint8_t *);

	void Update(float);

	int getFrameRate(int, Uint32);
	void limitFrameRate();

private:
	// window and renderer field
	Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;

	// Game Tick and FPS field
	int SCREEN_FPS = 60;
	int SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
	bool m_isFrameLimitEnabled = false;
	// Timer field
	Window window;
	Timer m_fpsTimer;
	Timer m_capTimer;
	unsigned int m_countedFrames = 0;
};