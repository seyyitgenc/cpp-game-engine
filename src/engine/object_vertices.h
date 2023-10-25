#pragma once
#include <vector>


// initialize objects that will be used by game
// --------------------------------------------
void App::initEntities() {
}

float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::vector<float> planeVertices = {
    // positions         // texcoords
    5.0f, -0.5f, 5.0f,    1.0f, 0.0f,
    -5.0f, -0.5f, 5.0f,   0.0f, 0.0f,
    -5.0f, -0.5f, -5.0f,  0.0f, 1.0f,
    5.0f, -0.5f, -5.0f,   1.0f, 1.0f
};

// std::vector<float> planeVertices = {
//     // positions          // normals         // texcoords
//     5.0f, -0.5f, 5.0f,    0.0f, 1.0f, 0.0f,   5.0f, 0.0f,
//     -5.0f, -0.5f, 5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
//     -5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
//     5.0f, -0.5f, -5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 5.0f
// };


std::vector<unsigned int> planeIndices = {
    0, 1, 2,
    0, 2, 3
};

std::vector<float> cubeVertices = {
      // positions
    -1.0f, 1.0f,  -1.0f, 
    -1.0f, -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f,
    1.0f,  1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  1.0f,
    -1.0f, -1.0f, 1.0f,
    
    1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, 1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  -1.0f,
    1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f,  1.0f,  
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    
    
    -1.0f, 1.0f,  -1.0f,
     1.0f,  1.0f,  -1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    -1.0f, 1.0f,  1.0f, 
    -1.0f, 1.0f,  -1.0f,
    
    
    -1.0f, -1.0f, -1.0f, 
    -1.0f, -1.0f, 1.0f, 
    1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 
    -1.0f, -1.0f, 1.0f, 
    1.0f,  -1.0f, 1.0f};