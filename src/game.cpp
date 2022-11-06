#include <headers/game.h>
#include <string>
Game::Game()
{
}
// TODO : WRITE DOWN BETTER DESTRUCT FUNCTION
Game::~Game()
{
	window.~Window();
	SDL_Quit();
	TTF_Quit();
}
Sprite s1;
Sprite s2;
TextTexture text;
void Game::Run()
{
	// window and renderer initialiazation done here. Default size of window is 800x600
	// if you want to initialize window height and width use
	// setWindowHeight() and setWindowWidth() functions with window object;
	window.setScreenWidth(1024);
	window.setScreenHeight(768);
	window.Init("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WINDOW_RESIZABLE);

	float elapsedTime = 0;
	float deltaTime = 0;
	float beforeTime = 0;
	float afterTime = 0;
	int frame = 0;

	float initialTime = SDL_GetTicks();
	// Initialize SDL_ttf
	if (TTF_Init() == -1)
	{
		printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
	}
	else
	{
		s1.initTexture("assets/texture.png", window.getRenderer());
		s2.initTexture("assets/stretch.bmp", window.getRenderer());
		SDL_Rect renderQuad = {0, 0, 28, 28};

		while (window.isOpen())
		{
			beforeTime = SDL_GetTicks();
			this->handleEvents();
			this->handleKeyStates(SDL_GetKeyboardState(NULL));
			// TODO : SEND 2 RECT TO LIMIT TEXTURE SIZE
			limitFrameRate();
			frame++;
			elapsedTime = SDL_GetTicks() - initialTime;
			afterTime = SDL_GetTicks();
			deltaTime = afterTime - beforeTime;
			text.loadRenderedText(std::to_string((int)(frame / (elapsedTime / 1000.f))), {255, 255, 255}, window.getRenderer());
			window.Draw(text.getTexture(), renderQuad);
			text.freeTexture();
		}
	}
	text.freeTexture();
	text.closeFont();
	s1.freeTexture();
	s2.freeTexture();
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