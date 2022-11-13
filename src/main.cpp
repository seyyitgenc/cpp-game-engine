#include "Engine/Game.h"

#include "ECS/ECS.h"
#include "ECS/Entity/Entity.h"
#include "ECS/Components/Transform.h"
#include "ECS/Entity/EntityManager.h"

#undef main

int main(int argc, char **argv)
{
	Game *game = nullptr;
	game = new Game();
	game->Run();//main game loop
	game->~Game();

	return EXIT_SUCCESS;
}