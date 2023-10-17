cmake_minimum_required(VERSION 3.5)

project(engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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

# set(LIBS ${GLFW3_LIBRARY} GL pthread freetype ${ASSIMP_LIBRARY})
# set me a LIBS for linux
set(LIBS glfw GL pthread freetype ${ASSIMP_LIBRARIES})

# configure file to get root directory
configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

# stb_image
add_library(STB_IMAGE "src/external/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

# glad
add_library(GLAD "src/external/glad/glad.c")
set(LIBS ${LIBS} GLAD)

# ImGui
add_library(IMGUI "src/external/ImGui/imgui.cpp")
set(LIBS ${LIBS} IMGUI)


# copy resources file
file(COPY ${CMAKE_CURRENT_LIST_DIR}/resources DESTINATION ${CMAKE_SOURCE_DIR}/bin)
# copy shaders file
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_SOURCE_DIR}/bin)

#add all executables
file(GLOB_RECURSE SOURCE 
    src/engine/*.cpp
    src/ECS/*.cpp
    src/external/ImGui/*.cpp
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
    