cmake_minimum_required(VERSION 3.0.0)

project(engine CXX)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_LIST_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#add all executables
file(GLOB_RECURSE SOURCE 
    Source/Engine/*.cpp
    Source/ECS/*.cpp
    Source/ImGui/*.cpp
    Source/ImGui/*.h
    )

    
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)

#include directories
set (INC_DIRECTORIES
    ${PROJECT_SOURCE_DIR}/Source
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_IMAGE_INCLUDE_DIRS} 
)
    
    
set(SDL_LIBRARIES ${SDL2_LIBRARIES} SDL2_image SDL2_ttf SDL2_mixer)

add_executable(${PROJECT_NAME} Source/main.cpp ${SOURCE}) 
include_directories(${PROJECT_NAME}  ${INC_DIRECTORIES})
target_link_libraries(${PROJECT_NAME}  ${SDL_LIBRARIES})
    