cmake_minimum_required(VERSION 3.0.0)

project(engine CXX)

#include directories
set (INC_DIRECTORIES
    ${PROJECT_SOURCE_DIR}/Source
)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_LIST_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#add all executables
file(GLOB_RECURSE SOURCE 
    Source/Engine/*.cpp
    Source/ECS/*.cpp)


add_executable(${PROJECT_NAME} Source/main.cpp ${SOURCE}) 

include(FetchContent)

# must be ON after building the project
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET FALSE)
# sdl2
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL
    GIT_TAG         release-2.26.x
    GIT_PROGRESS    TRUE
)

# sdl2_image
FetchContent_Declare(
    SDL2_IMAGE
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL_image
    GIT_TAG         main
    GIT_PROGRESS    TRUE
)
# sdl2_ttf
FetchContent_Declare(
    SDL2_TTF
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL_ttf
    GIT_TAG         main
    GIT_PROGRESS    TRUE
)

# sdl2_ttf
FetchContent_Declare(
    SDL2_MIXER
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG         main
    GIT_PROGRESS    TRUE
)

FetchContent_MakeAvailable(SDL2 SDL2_IMAGE SDL2_TTF SDL2_MIXER)

set(SDL_LIBRARIES ${SDL_LIBRARIES} SDL2 SDL2main SDL2_image SDL2_ttf SDL2_mixer)

target_include_directories("${PROJECT_NAME}" PRIVATE ${INC_DIRECTORIES})
target_link_libraries("${PROJECT_NAME}" PRIVATE ${SDL_LIBRARIES})
