#include <game.hpp>
#undef main

Game *game = nullptr;
int main()
{
	game = new Game();
	//initializing window values
	game->Init("SDL Project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 500, 500, SDL_WINDOW_RESIZABLE);
	game->constructGame();//construct game
	game->destructGame();//destruct game
	return EXIT_SUCCESS;
}