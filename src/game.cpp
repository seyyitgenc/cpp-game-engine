#include <headers/game.h>
Game::Game()
{
}
Game::~Game()
{
	SDL_DestroyRenderer(window.getRenderer());
	SDL_DestroyWindow(window.getWindow());
	window.close();
	SDL_Quit();
}
void Game::Run()
{
	if (!window.Init("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1366, 768, SDL_WINDOW_RESIZABLE))
		std::cout << "Error Occured" << std::endl;
	else
	{
		if (!window.loadMedia())
			std::cout << "Failed to load image" << std::endl;
		else
		{
			while (window.isOpen())
			{
				Uint64 start = SDL_GetPerformanceCounter();
				this->handleEvents();
				this->handleKeyStates(SDL_GetKeyboardState(NULL));
				Uint64 end = SDL_GetPerformanceCounter();
				float elapsedMS = (end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
				this->Update(elapsedMS);
				window.Render();
			}
		}
	}
}

void Game::handleKeyStates(const std::uint8_t *keystates)
{
	if (keystates[SDL_SCANCODE_W])
	{
		std::cout << "W" << std::endl;
	}
	if (keystates[SDL_SCANCODE_S])
	{
		std::cout << "S" << std::endl;
	}
}

void Game::handleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
			window.setisOpen(false);
		else if (event.type == SDL_KEYDOWN)
		{
			if (event.key.keysym.sym == SDLK_a)
				std::cout << "a" << std::endl;
			// for starting timer manually
			//  if (event.key.keysym.sym == SDLK_s)
			//  	if (timer.isStarted())
			//  		timer.stop();
			//  	else
			//  		timer.start();
			//  else if (event.key.keysym.sym == SDLK_p)
			//  	if (timer.isPaused())
			//  		timer.unpause();
			//  	else
			//  		timer.pause();
		}
	}
}
void Game::Update(float elapsedMS)
{
	// DO UPDATE
}

int Game::getFrameRate(int countedFrames, Uint32 fpsTimer)
{
	float avgFps = countedFrames / (fpsTimer / 1000.f);
	if (avgFps > 2000000)
		avgFps = 0;
	return avgFps;
}

/// @brief this function will set framerate for your game. You need to use this function
///	with limitFrameRate() function to apply limitation
/// @param SCREEN_FPS set your FPS value here
/// @param isFrameLimitEnabled this parameter specifies if frame limitiation is true/false
void Game::setScreenFps(int SCREEN_FPS, bool isFrameLimitEnabled)
{
	if (isFrameLimitEnabled)
		SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
	else
		SCREEN_TICKS_PER_FRAME = 1000.f / SDL_GetPerformanceCounter();
}
// limits the frame rate according to SCREEN_FPS
// this function is good to use with setScreenFps() function
void Game::limitFrameRate()
{
	m_capTimer.start();
	int frameTicks = m_capTimer.getTicks();
	if (frameTicks < SCREEN_TICKS_PER_FRAME)
		SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
}