#pragma once

#include "model.h"
#include <vector>

struct FrameBuffer
{
public:
    void configure(int width, int height);
    void addTextureType();
    void bind();
private:
    std::vector<Texture> textures;
};

class Scene
{
public:
    Scene() = default;
    ~Scene() = default;
private:
    std::vector<Model> listOfObjects;
    // std::vector<Decal> listOfObjects;
    // Camera* activeCam = nullptr; // default camera pointer
};

class Renderer
{
private:
    
public:
    Renderer() = default;
    ~Renderer() = default;
};