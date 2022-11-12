#include <headers/game.h>

#include "headers/ECS/ECS.h"
#include "headers/ECS/Entity.h"
#include "headers/ECS/Transform.h"
#include "headers/ECS/EntityManager.h"

#undef main

Game *game = nullptr;
int main(int argc, char **argv)
{
	EntityManager * manager=new EntityManager();
	Entity *e1 = new Entity();
	manager->addEntity(e1);
	std::cout << e1->getComponent<Transform>().position << std::endl;
	game = new Game();
	game->Run(); // run game
	game->~Game();
	return EXIT_SUCCESS;
}