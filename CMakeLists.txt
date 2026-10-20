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
    set(LIBS glfw pthread ${ASSIMP_LIBRARIES})
endif(WIN32)

if(UNIX)
    # assimp
    add_subdirectory(src/external/assimp)

    # Linux
    message(STATUS "Linux detected")
    set(LIBS glfw pthread assimp)
endif(UNIX)


# configure file to get root directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/configuration/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration/config.h)
include_directories(${CMAKE_SOURCE_DIR}/src/configuration)

# stb_image
add_library(STB_IMAGE "src/external/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

# Dear ImGui, docking branch, vendored as plain files (not a submodule).
# Copied from https://github.com/ocornut/imgui at docking commit d88f238
# (1.93.1 WIP). To update, copy the same files over from a newer docking
# checkout: the root sources and headers, LICENSE.txt, and the two backends.
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/external/ImGui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)
set(LIBS ${LIBS} imgui)

#add all executables
file(GLOB_RECURSE SOURCE
    src/engine/*.cpp
    src/engine/*.hpp
    src/engine/*.h
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

set(ENGINE_OUTPUT_DIR ${CMAKE_BINARY_DIR})
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(ENGINE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/debug")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(ENGINE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/release")
endif(CMAKE_BUILD_TYPE MATCHES Debug)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ENGINE_OUTPUT_DIR}/")

# Shaders
# Every shaders/*.slang file holds a vertMain and a fragMain entry point, and
# compiles to <name>.vert.spv and <name>.frag.spv, which is what
# Resource/Types/Shader.hpp loads. shaders/common/ holds modules the others
# import; they are not compiled on their own. Output goes next to the
# executable, because Shader loads "shaders/..." relative to the working
# directory.
find_program(SLANGC slangc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
if(NOT SLANGC)
    message(FATAL_ERROR "slangc not found. It ships with the Vulkan SDK; check that VULKAN_SDK is set.")
endif()

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${ENGINE_OUTPUT_DIR}/shaders)
file(GLOB SHADER_SOURCES ${SHADER_SOURCE_DIR}/*.slang)
file(GLOB SHADER_MODULES ${SHADER_SOURCE_DIR}/common/*.slang)

set(SHADER_BINARIES)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    foreach(STAGE vert frag)
        if(STAGE STREQUAL "vert")
            set(ENTRY_POINT vertMain)
            set(SLANG_STAGE vertex)
        else()
            set(ENTRY_POINT fragMain)
            set(SLANG_STAGE fragment)
        endif()

        set(SHADER_BINARY ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.${STAGE}.spv)
        add_custom_command(
            OUTPUT ${SHADER_BINARY}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
            COMMAND ${SLANGC} ${SHADER} -target spirv -profile spirv_1_4
                    -entry ${ENTRY_POINT} -stage ${SLANG_STAGE}
                    -I ${SHADER_SOURCE_DIR} -o ${SHADER_BINARY}
            DEPENDS ${SHADER} ${SHADER_MODULES}
            COMMENT "Compiling shader ${SHADER_NAME}.${STAGE}.spv"
            VERBATIM)
        list(APPEND SHADER_BINARIES ${SHADER_BINARY})
    endforeach()
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_BINARIES})
add_dependencies(${PROJECT_NAME} shaders)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
# target_compile_options(${PROJECT_NAME} PRIVATE -std=c++23)
# target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined,leak)
