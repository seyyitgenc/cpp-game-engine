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
	engine->init();
	while (engine->isRunning())
	{
		engine->events();
		engine->render();
		engine->update();
	}
}

void Game::Update(float elapsedMS)
{
	// DO UPDATE
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