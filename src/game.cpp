#include <headers/game.h>
#include <vector>
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
typedef struct point
{
	int x;
	int y;
} point;
std::vector<point> points;
void addPoints(const int &x, const int &y)
{
	point p;
	p.x = x;
	p.y = y;
	points.push_back(p);
}

void drawPoints(SDL_Renderer *renderer)
{

	if (points.size() < 2)
	{
		// do nothing
	}
	else
	{
		for (size_t i = 1; i < points.size(); ++i)
		{
			SDL_Rect p1 = {points[i - 1].x, points[i - 1].y, 10, 10};
			SDL_Rect p2 = {points[i].x, points[i].y, 10, 10};
			SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
			SDL_RenderFillRect(renderer, &p1);
			SDL_RenderFillRect(renderer, &p2);

			SDL_SetRenderDrawColor(renderer, 255, 0, 155, 255);
			SDL_RenderDrawLine(renderer, points[i - 1].x, points[i - 1].y, points[i].x, points[i].y);
		}
		if (points.size() > 2)
		{
			SDL_RenderDrawLine(renderer, points[0].x, points[0].y, points[points.size() - 1].x, points[points.size() - 1].y);
		}
	}
}
// TODO : Refactor this section aswell
vf2d velocity = {0.0f, 0.0f};
double mov = 1.5f;

int array[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
std::vector<SDL_Rect> tilemap;

// TODO : Move this section
void Game::Run()
{
	TextToTexture fpsCounter;
	TextToTexture t1;
	Sprite s1;
	// initialize default sized window 800x600

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 12; j++)
		{
			if (array[i + j * 16] == 1)
			{
				SDL_Rect tile = {i * 64, j * 64, 64, 64};
				tilemap.push_back(tile);
			}
		}
	}
	if (window.Init("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WINDOW_RESIZABLE))
	{
		s1.setPos(100, 100);
		if (TTF_Init() == -1)
		{
			printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
		}
		else
		{
			// TODO: Refactor this section
			double initTime = SDL_GetTicks();
			double elapsedTime;
			Uint64 NOW = SDL_GetPerformanceCounter();
			Uint64 LAST = 0;
			float dt = 0;
			s1.initTexture("assets/texture.png", window.getRenderer());
			int frames = 0;
			TextToTexture::setFont("fonts/aerial.ttf");
			// main loop
			SDL_FRect test = {400, 300, 64, 64};
			while (window.isOpen())
			{
				LAST = NOW;
				NOW = SDL_GetPerformanceCounter();
				dt = ((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
				// std::cout << dt << std::endl;
				velocity = {0, 0};
				limitFrameRate();
				frames++;
				// time in seconds
				elapsedTime = SDL_GetTicks() - initTime;
				// this->Update(deltaTime);

				// frame rate counter
				fpsCounter.loadRenderedText(std::to_string(getFrameRate(frames, elapsedTime)), fpsCounter.getTextColor(), window.getRenderer());
				t1.loadRenderedText(std::to_string(timer.getTicks() / 1000.0f), t1.getTextColor(), window.getRenderer());
				t1.setPos({800 - t1.getWidth(), 0});
				window.setRendererColor(0, 0, 0, 255);
				SDL_RenderClear(window.getRenderer());
				drawPoints(window.getRenderer());
				// render tile map
				// TODO : create function for it
				// for newly created rectangle color
				window.setRendererColor(255, 255, 0, 255);
				for (auto t : tilemap)
				{
					SDL_RenderFillRect(window.getRenderer(), &t);
				}

				handleEvents(SDL_GetKeyboardState(NULL), dt);
				test.x += velocity.x;
				test.y += velocity.y;
				// drawPoints(window.getRenderer());
				window.setRendererColor(255, 0, 0, 255);
				SDL_RenderFillRectF(window.getRenderer(), &test);
				window.Draw(t1);
				window.Draw(fpsCounter, fpsCounter.getPos(), fpsCounter.getSize(), NULL, 0.0, NULL, SDL_FLIP_NONE);
				SDL_RenderPresent(window.getRenderer());
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

void Game::handleEvents(const std::uint8_t *keystates, const double &dt)
{
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0)
	{
		if (event.type == SDL_QUIT)
			window.setisOpen(false);
		if (event.type == SDL_WINDOWEVENT_RESIZED)
		{
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				int x, y;
				SDL_GetMouseState(&x, &y); // return mouse offset
				addPoints(x, y);
			}
			else if (event.button.button == SDL_BUTTON_RIGHT)
			{
			}
		}
		else if (event.type == SDL_KEYDOWN)
		{
			// if (event.key.keysym.sym == SDLK_s)
			// {

			// 	timer.isStarted() ? timer.stop() : timer.start();
			// }
			// else if (event.key.keysym.sym == SDLK_p)
			// {
			// 	timer.isPaused() ? timer.unpause() : timer.pause();
			// }
		}
	}
	if (keystates[SDL_SCANCODE_W])
	{
		velocity.y -= mov * dt;
	}
	if (keystates[SDL_SCANCODE_S])
	{
		velocity.y += mov * dt;
	}
	if (keystates[SDL_SCANCODE_D])
	{
		velocity.x += mov * dt;
	}
	if (keystates[SDL_SCANCODE_A])
	{
		velocity.x -= mov * dt;
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

// this function will set framerate for your game. You need to use this function
//	with limitFrameRate() function to apply limitation
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