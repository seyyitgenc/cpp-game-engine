// sdl
#ifndef sdl_h
#define sdl_h
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#endif
// timer
#ifndef timer_h
#define timer_h
#include <headers/timer.h>
#endif
#ifndef entity_h
#define enitity_h
#include <headers/entity.h>
#endif
// window
#ifndef window_h
#define window_h
#include <headers/window.h>
#endif

// headers initialized once
#include <string>
#include <iostream>

class Game
{
public:
	Game();
	~Game();
	void Run();										  // run main loop
	void setScreenFps(int, bool);					  // set screen fps
	void handleEvents(const std::uint8_t *keystates,const double &dt); // handles events created by user
	void Update(float dt);							  // updates game physics
	int getFrameRate(int, Uint32);					  // get frame rate
	void limitFrameRate();							  // limits the fps to the fps limit
public:
	Clock timer;

private:
	// window and renderer field
	Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
	Window window;
	// Game Tick and FPS field
	int SCREEN_FPS = 120;
	int SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
	bool m_isFrameLimitEnabled = false;
	// Timer field
	Clock m_capTimer;
	unsigned int m_countedFrames = 0;
};