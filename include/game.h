#pragma once
#include <SDL2/SDL.h>
#include <iostream>
#include <timer.h>
class Game
{
public:
	Game();
	~Game();
	void constructGame();
	void Init(const char *, int, int, int, int, int);

	void setFullScreen();
	void setRenderColor(Uint8, Uint8, Uint8, Uint8);
	void setScreenFps(int, bool);

	void handleEvents();
	void handleKeyStates(const std::uint8_t *);

	void Update();
	void Render();

	int getFrameRate(int, Uint32);
	double getDeltaTime();
private:
	// window and renderer field
	Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
	bool m_isRunning = false;
	bool m_fullscreen = false;
	SDL_Renderer *m_renderer = nullptr;
	SDL_Window *m_window = nullptr;
	int m_flags = 0;

	// Game Tick and FPS field
	int SCREEN_FPS = 1;
	int SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
	bool m_isFrameLimitEnabled = false;
	// Timer field
	Uint32 NOW = 0, LAST = 0;
	LTimer time;
	LTimer m_fpsTimer;
	LTimer m_capTimer;
	int m_countedFrames;
};