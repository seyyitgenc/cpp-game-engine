#include <cstdlib>
#include <exception>

#include "App.hpp"

#include "Utils/FileSystem.hpp"
#include "Utils/Log.hpp"

int main(int argc, char* argv[])
{
    Log::setLevel(Log::Debug);

    try {
        // Before anything else, while the working directory is still the one
        // argv[0] is relative to.
        FileSystem::init(argc > 0 ? argv[0] : nullptr);
        Log::write(Log::Info, "Resource root: ", FileSystem::getRoot().string(), "\n");

        GNC::App app;
        app.Run();
    } catch (const std::exception& e) {
        Log::write(Log::Fatal, RED_TEXT("FATAL: "), e.what(), "\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
