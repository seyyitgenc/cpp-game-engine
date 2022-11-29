#include "Engine/Game.h"
Game::Game()
{
}
Game::~Game()
{
	engine->clean();
}
void Game::Run()
{
	engine = &Engine::get();
	// TODO : ADD COLLISION (AABB)
	engine->initApp();
	engine->initEntities();
	while (engine->isRunning())
	{
		// limitFrameRate();
		engine->events();
		engine->update(getDeltaTime());
		engine->render();
		// std::cout << getDeltaTime() << std::endl;
	}
}

int Game::getFrameRate(const int &countedFrames, const Uint32 &fpsTimer)
{
	float avgFps = countedFrames / (fpsTimer / 1000.f);
	if (avgFps > 2000000)
		avgFps = 0;
	return avgFps;
}
float &Game::getDeltaTime()
{
	static Uint64 NOW = SDL_GetPerformanceCounter();
	static Uint64 LAST = 0;
	static float dt;
	LAST = NOW;
	NOW = SDL_GetPerformanceCounter();
	dt = (NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency();
	return dt;
}
void Game::setFrameRate(const int &SCREEN_FPS)
{
	SCREEN_TICKS_PER_FRAME = 1000.f / SCREEN_FPS;
}
void Game::limitFrameRate()
{
	m_capTimer.start();
	int frameTicks = m_capTimer.getTicks();
	if (frameTicks < SCREEN_TICKS_PER_FRAME)
		SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
}
// will handle this section later
/*
typedef struct point
{
	float x;
	float y;
} point;
std::vector<point> points;
void addPoints(const int &x, const int &y)
{
	point p;
	p.x = x;
	p.y = y;
	points.push_back(p);
}
float force = 1.0f;
vf2d test = {0, 0};
void drawPoints(SDL_Renderer *renderer)
{

	if (points.size() < 2)
	{
	}
	else
	{
		for (size_t i = 1; i < points.size(); ++i)
		{
			for (size_t i = 0; i < points.size(); i++)
			{
				points[i].x += test.x;
				points[i - 1].x += test.x;
			}
			SDL_SetRenderDrawColor(renderer, 255, 0, 155, 255);
			SDL_RenderDrawLine(renderer, points[i - 1].x, points[i - 1].y, points[i].x, points[i].y);
		}
		if (points.size() > 2)
		{
			SDL_RenderDrawLine(renderer, points[0].x, points[0].y, points[points.size() - 1].x, points[points.size() - 1].y);
		}
		test.x = 0;
		test.y = 0;
	}
}

void applyForce(double dt)
{
	test.x += force * dt;
}
*/