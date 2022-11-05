#include <headers/game.h>
#undef main

Game *game = nullptr;
int main()
{
	// this constructor initialize default window
	game = new Game();
	game->Run(); // run game
	return EXIT_SUCCESS;
}