#include <headers/game.h>
#undef main

Game *game = nullptr;
int main()
{
	// this constructor initialize default window
	game = new Game();
	game->Run(); // run game
	game->~Game();
	return EXIT_SUCCESS;
}