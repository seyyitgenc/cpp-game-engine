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

void Game::initEntities(){
	
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