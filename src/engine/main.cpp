#include "app.h"

int main() {
    // note : this doesn't seems right
    App *game;
    game = &App::get();
    game->run();  // main game loop
    game->~App();
    return 0;
}
