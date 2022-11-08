#include <headers/game.h>
Game::Game()
{
}
// TODO : WRITE DOWN BETTER DESTRUCT FUNCTION
Game::~Game()
{
	window.~Window(); // destruct everything after window closed
	TTF_Quit();
	SDL_Quit();
}
void Game::Run()
{
	TextToTexture text;
	Sprite s1;
	vi2d pos = {64, 64};
	vi2d size = {32, 64};
	// initialize default sized window 800x600
	if (window.Init("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WINDOW_RESIZABLE))
	{
		if (TTF_Init() == -1)
		{
			printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
		}
		else
		{
			// TODO : Move draw functions into the window class and try to use templates
			Clock timer;
			Uint32 initTime = SDL_GetTicks();
			Uint32 elapsedTime;
			s1.initTexture("assets/texture.png", window.getRenderer());
			int frames = 0;

			text.loadFont("fonts/aerial.ttf");
			// main loop
			text.loadRenderedText("zart zurt kart kurt", text.getTextColor(), window.getRenderer());
			while (window.isOpen())
			{
				this->handleEvents();
				this->handleKeyStates(SDL_GetKeyboardState(NULL));
				limitFrameRate();
				frames++;
				elapsedTime = SDL_GetTicks() - initTime;
				// this->Update(deltaTime);
				SDL_RenderClear(window.getRenderer());
				window.setRendererColor(0, 0, 255, 255);
				window.Draw(s1, pos, size, NULL, 0.0, NULL, SDL_FLIP_NONE, window.getRenderer());
				window.Draw(text,text.getTexturePos(), text.getTextureSize(), NULL, 0.0, NULL, SDL_FLIP_NONE, window.getRenderer());
				SDL_RenderPresent(window.getRenderer());
			}
		}
	}
	else
	{
		std::cout << "Somethings Wrong" << std::endl;
	}
	text.freeTexture();
	text.closeFont();
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
			{
			}
			else if (event.key.keysym.sym == SDLK_s)
			{
			}
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