#include <headers/game.h>
Game::Game()
{
}
Game::~Game()
{
	window.~Window(); // destruct everything after window closed
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}
void Game::Run()
{
	TextToTexture fpsCounter;
	TextToTexture t1;
	Sprite s1;
	s1.setPos(100, 100);
	// initialize default sized window 800x600
	if (window.Init("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WINDOW_RESIZABLE))
	{
		if (TTF_Init() == -1)
		{
			printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
		}
		else
		{
			// TODO: Move draw functions into the window class and try to use templates
			Uint32 initTime = SDL_GetTicks();
			Uint32 elapsedTime;
			s1.initTexture("assets/texture.png", window.getRenderer());
			int frames = 0;
		
			TextToTexture::setFont("fonts/aerial.ttf");
			// main loop
			while (window.isOpen())
			{
				limitFrameRate();
				frames++;
				elapsedTime = SDL_GetTicks() - initTime;
				// this->Update(deltaTime);
				SDL_RenderClear(window.getRenderer());
				window.setRendererColor(0, 0, 255, 255);
				// frame rate counter
				fpsCounter.loadRenderedText(std::to_string(getFrameRate(frames, elapsedTime)), fpsCounter.getTextColor(), window.getRenderer());
				t1.loadRenderedText(std::to_string(timer.getTicks() / 1000.0f), t1.getTextColor(), window.getRenderer());
				t1.setPos({800 - t1.getWidth(), 0});
				window.Draw(s1);
				window.Draw(t1);
				window.Draw(fpsCounter, fpsCounter.getPos(), fpsCounter.getSize(), NULL, 0.0, NULL, SDL_FLIP_NONE);
				SDL_RenderPresent(window.getRenderer());
				this->handleEvents();
			}
		}
	}
	else
	{
		std::cout << "Somethings Wrong" << std::endl;
	}
	TextToTexture::closeFont();
	t1.freeTexture();
	fpsCounter.freeTexture();
}

void Game::handleKeyStates(const std::uint8_t *keystates)
{
	// if (keystates[SDL_SCANCODE_W])
	// {
	// 	std::cout << "W" << std::endl;
	// }
	// if (keystates[SDL_SCANCODE_S])
	// {
	// 	std::cout << "S" << std::endl;
	// }
}

void Game::handleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0)
	{
		if (event.type == SDL_QUIT)
			window.setisOpen(false);
		if (event.type == SDL_WINDOWEVENT_RESIZED)
		{
		}
		else if (event.type == SDL_KEYDOWN)
		{
			if (event.key.keysym.sym == SDLK_s)
			{

				timer.isStarted() ? timer.stop() : timer.start();
			}
			else if (event.key.keysym.sym == SDLK_p)
			{
				timer.isPaused() ? timer.unpause() : timer.pause();
			}
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