#pragma once
#include "shader.h"
#include "texture.h"

#include "util/helpers.hpp"

#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    std::vector<Vertex> verticies;
    std::vector<unsigned int> indices;
    std::vector<std::string> texture_names;
    bool hasIndices;
    // bool hasTextures;
    bool hasNormals;
    bool hasTexCoords;
    unsigned int VAO, VBO, EBO;

public:
    Mesh(std::vector<Vertex> verticies, std::vector<unsigned int> indices, std::vector<std::string> texture_names,
        bool hasIndices = false, bool hasNormals = false, bool hasTexCoords = false);
    ~Mesh() = default;

    void Draw(const Shader& shader) const;

private:
    void setupMesh();
};
