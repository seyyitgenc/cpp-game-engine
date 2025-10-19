#include "app.h"

int main() {
    Log::setLevel(Log::Debug);

    // note : this doesn't seems right
    App::getInstance().run();
    return 0;
}
