#include <SDL.h>
#include <iostream>
class Game
{
public:
	Game();
	~Game();
	void Init(const char *, int, int, int, int, int);
	void handleEvents();
	void render();
	void update();
	void setFullScreen();
	void setRenderColor(Uint8, Uint8, Uint8, Uint8);

	bool isRunnig() { return isOpen; }

private:
	Uint8 r = 10, g = 10, b = 10, a = 0;
	bool isOpen = false;
	bool fullscreen = false;
	SDL_Renderer *renderer;
	SDL_Window *window;
	int flags = 0;
};