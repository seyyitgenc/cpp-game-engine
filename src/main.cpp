#include "game.h"
#undef main

Game *game = nullptr;

int main()
{
	game = new Game();
	game->Init("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 500, 500, SDL_WINDOW_RESIZABLE);
	game->setRenderColor(0, 0, 0, 0);
	int w = 0, s = 0;
	while (game->isRunnig())
	{
		game->handleEvents();
		game->render();
		game->update();
		const std::uint8_t *keystates = SDL_GetKeyboardState(nullptr);
		if (keystates[SDL_SCANCODE_W])
		{
			w++;
			std::cout << w << std::endl;
		}
		if (keystates[SDL_SCANCODE_S])
		{
			s++;
			std::cout << s << std::endl;
		}
	}
	game->~Game();
	return 0;
}
// simple update function
void update()
{
	// if things could update the code would go in here.
}