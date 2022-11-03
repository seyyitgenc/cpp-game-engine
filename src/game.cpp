#include <game.h>
#include <timer.h>
Game::Game()
{
}
Game::~Game()
{
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}
void Game::constructGame()
{
	double deltaTime = 0;
	setScreenFps(60, true);
	m_fpsTimer.start();
	while (this->m_isRunning)
	{
		m_capTimer.start();
		this->handleEvents();
		this->Render();
		this->Update();
		int fps = this->getFrameRate(++m_countedFrames, m_fpsTimer.getTicks());
		int frameTicks = m_capTimer.getTicks();
		if (frameTicks < SCREEN_TICKS_PER_FRAME)
		{
			// Wait remaining time
			SDL_Delay(this->SCREEN_TICKS_PER_FRAME - frameTicks);
		}
		deltaTime = getDeltaTime();

		std::cout << deltaTime << std::endl;
	}
}

void Game::Init(const char *title, int x, int y, int w, int h, int flags)
{
	m_flags = flags;
	if (m_fullscreen)
		m_flags = flags | SDL_WINDOW_FULLSCREEN;
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		std::cout << "Subsystems Initialized\n";
		m_window = SDL_CreateWindow(title, x, y, w, h, flags);
		if (m_window)
		{
			std::cout << "Window Created!\n";
			SDL_SetWindowMinimumSize(m_window, 100, 100);
		}
		m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
		if (m_renderer)
		{
			SDL_SetRenderDrawColor(m_renderer, m_red, m_green, m_blue, m_alpha);
			std::cout << "Renderer Created!\n";
			SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
			m_isRunning = true;
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
			m_isRunning = false;
		else if (event.type == SDL_KEYDOWN)
		{
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
void Game::setRenderColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
	m_red = red;
	m_blue = blue;
	m_green = green;
	m_alpha = alpha;
}

void Game::Render()
{
	SDL_SetRenderDrawColor(m_renderer, m_red, m_green, m_blue, m_alpha);
	SDL_RenderClear(m_renderer);

	SDL_RenderPresent(m_renderer);
}

void Game::Update()
{
}

void Game::setFullScreen()
{
	m_flags = m_flags | SDL_WINDOW_FULLSCREEN;
}
int Game::getFrameRate(int countedFrames, Uint32 fpsTimer)
{
	float avgFps = countedFrames / (fpsTimer / 1000.f);
	if (avgFps > 2000000)
		avgFps = 0;
	return (int)avgFps;
}
void Game::setScreenFps(int SCREEN_FPS, bool isFrameLimitEnabled)
{
	if (isFrameLimitEnabled)
		this->SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
	// else use default value
}
double Game::getDeltaTime()
{
	LAST = NOW;
	time.start();
	NOW = time.getTicks();
	time.reset();
	NOW = time.getTicks();
	double dt = (NOW - LAST) / 1000.f;
	return dt;
}