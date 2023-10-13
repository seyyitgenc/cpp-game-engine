#include "app.h"

int main() {
    App *game;
    game = &App::get();
    game->run();  // main game loop
    game->~App();
    return 0;
}
