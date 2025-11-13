#include "app.hpp"

#include "util/log.hpp"

int main()
{
    Log::setLevel(Log::Debug);

    // note : this doesn't seems right
    App::instance().run();
    return 0;
}
