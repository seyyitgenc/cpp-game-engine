cmake_minimum_required(VERSION 3.5)
# todo: set binary directory
project(engine)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)")
endif(NOT CMAKE_BUILD_TYPE)

# find required libs

# OpenGL
find_package(OpenGL REQUIRED)

# GLM
find_package(glm REQUIRED)
message(STATUS "Found GLM in ${glm_INCLUDE_DIRS}")

#assimp
find_package(assimp REQUIRED)
message(STATUS "Found assimp in ${ASSIMP_INCLUDE_DIRS}")

# GLFW3
find_package(glfw3 REQUIRED)
message(STATUS "Found GLFW3 in ${glfw3_INCLUDE_DIRS}")

set(LIBS glfw3 opengl32 freetype ${ASSIMP_LIBRARIES})
# set me a LIBS for linux
# set(LIBS glfw GL pthread freetype ${ASSIMP_LIBRARIES})

# configure file to get root directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/configuration/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration/config.h)
include_directories(${CMAKE_SOURCE_DIR}/src/configuration)

# stb_image
add_library(STB_IMAGE "src/external/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

# glad
add_library(GLAD "src/external/glad/glad.c")
set(LIBS ${LIBS} GLAD)

# ImGui
add_library(IMGUI "src/external/ImGui/imgui.cpp")
set(LIBS ${LIBS} IMGUI)

#add all executables
file(GLOB_RECURSE SOURCE 
    src/engine/*.cpp
    src/engine/*.hpp
    src/engine/*.h
    
    src/external/ImGui/*.cpp
    src/external/ImGui/*.h
    src/external/ImGui/*.hpp
    
    src/util/*.cpp
    src/util/*.hpp
    src/util/*.h

    src/renderer/*.cpp
    src/renderer/*.hpp
    src/renderer/*.h
    
    src/io/*.cpp
    src/io/*.hpp
    src/io/*.h
    )

#include directories
set (INC_DIRECTORIES
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/external
)

add_executable(${PROJECT_NAME} ${SOURCE})
include_directories(${PROJECT_NAME}  ${INC_DIRECTORIES})
target_link_libraries(${PROJECT_NAME} ${LIBS}) 

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/debug/")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/release/")
endif(CMAKE_BUILD_TYPE MATCHES Debug)
    