#include "game.h"

Game::Game()
{
}
Game::~Game()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
void Game::Init(const char *title, int x, int y, int w, int h, int flags)
{
	this->flags = flags;
	if (fullscreen)
		this->flags = flags | SDL_WINDOW_FULLSCREEN;
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		std::cout << "Subsystems Initialized";
		window = SDL_CreateWindow(title, x, y, w, h, flags);
		if (window)
		{
			std::cout << "Window Created!\n";
			SDL_SetWindowMinimumSize(window, 100, 100);
		}
		renderer = SDL_CreateRenderer(window, -1, 0);
		if (renderer)
		{
			SDL_SetRenderDrawColor(renderer, this->r, this->g, this->b, this->a);
			std::cout << "Renderer Created!\n";
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			isOpen = true;
		}
	}
}

void Game::handleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
			this->isOpen = false;
	}
}
void Game::setRenderColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	this->r = r;
	this->b = b;
	this->g = g;
	this->a = a;
}

void Game::render()
{
	SDL_SetRenderDrawColor(this->renderer, this->r, this->g, this->b, this->a);
	SDL_RenderClear(renderer);

	SDL_RenderPresent(renderer);
}

void Game::update()
{
}

void Game::setFullScreen()
{
	this->flags = this->flags | SDL_WINDOW_FULLSCREEN;
}
