#include "app.h"

int main() {
    // note : this doesn't seems right
    App::getInstance().run();
    App::getInstance().~App();
    return 0;
}
