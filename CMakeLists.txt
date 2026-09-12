cmake_minimum_required(VERSION 3.10)
# todo: set binary directory
project(engine)

set(CMAKE_CXX_STANDARD 23)
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

#assimp
message(STATUS "Found assimp in ${ASSIMP_INCLUDE_DIRS}")

# GLFW3
find_package(glfw3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIRS}")

# Vulkan
find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan in ${Vulkan_INCLUDE_DIR}")


if(WIN32)
    # assimp
    find_package(assimp REQUIRED)
    # Windows
    message(STATUS "Windows detected")
    set(LIBS glfw pthread freetype ${ASSIMP_LIBRARIES})
endif(WIN32)

if(UNIX)
    # assimp
    add_subdirectory(src/external/assimp)

    # Linux
    message(STATUS "Linux detected")
    set(LIBS glfw GL pthread freetype assimp)
endif(UNIX)


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
    ${PROJECT_SOURCE_DIR}/src/engine
    ${PROJECT_SOURCE_DIR}/src/external
    ${Vulkan_INCLUDE_DIR}
)

set(LIBS ${LIBS} Vulkan::Vulkan)

add_executable(${PROJECT_NAME} ${SOURCE})
include_directories(${PROJECT_NAME}  ${INC_DIRECTORIES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/debug/")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/release/")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
# target_compile_options(${PROJECT_NAME} PRIVATE -std=c++23)
# target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined,leak)
