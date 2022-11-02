#pragma once
#include <SDL2/SDL.h>
#include <iostream>
class Game
{
public:
	Game();
	~Game();
	void constructGame();
	void destructGame();
	void Init(const char *, int, int, int, int, int);
	void handleEvents();
	void handleKeyStates(const std::uint8_t *);
	void Render();
	void Update();
	void setFullScreen();
	void setRenderColor(Uint8, Uint8, Uint8, Uint8);

private:
	Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
	bool m_isRunning = false;
	bool m_fullscreen = false;
	SDL_Renderer *m_renderer = nullptr;
	SDL_Window *m_window = nullptr;
	int m_flags = 0;
	const int SCREEN_FPS = 120;
	const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;
};