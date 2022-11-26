#include "Engine/Game.h"

#undef main

int main(int argc, char **argv)
{
	Game *game = nullptr;
	game = new Game();
	game->Run(); // main game loop
	game->~Game();
	return EXIT_SUCCESS;
}