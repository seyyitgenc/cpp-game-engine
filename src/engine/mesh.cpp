#include "glad/glad.h"

#include "mesh.h"

#include "globals.h"

Mesh::Mesh(std::vector<Vertex> verticies, std::vector<unsigned int> indices, std::vector<std::string> texture_names,
    bool hasIndices, bool hasNormals, bool hasTexCoords)
    : verticies(verticies)
    , indices(indices)
    , texture_names(texture_names)
    , hasIndices(hasIndices)
    , hasNormals(hasNormals)
    , hasTexCoords(hasTexCoords)
{
    setupMesh();
}

void Mesh::Draw(const Shader& shader) const
{
    if (hasTexCoords) {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int depthNr = 1;
        unsigned int displacementNr = 1;
        unsigned int rougnessNr = 1;
        unsigned int reflectionNr = 1;
        for (size_t i = 0; i < texture_names.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            Texture* currentText = TextureManager::instance()->getTexture(texture_names[i]);
            std::string name;
            switch (currentText->type) {
            case TextureType::DIFFUSE:
                name = "texture_diffuse";
                number = std::to_string(diffuseNr++);
                break;
            case TextureType::SPECULAR:
                name = "texture_specular";
                number = std::to_string(specularNr++);
                break;
            case TextureType::NORMAL:
                name = "texture_normal";
                number = std::to_string(normalNr++);
                break;
            case TextureType::DEPTH:
                name = "texture_depth";
                number = std::to_string(depthNr++);
                break;
            case TextureType::ROUGNESS:
                name = "texture_roughness";
                number = std::to_string(rougnessNr++);
                break;
            case TextureType::REFLECTION:
                name = "texture_reflection";
                number = std::to_string(reflectionNr++);
                break;
            case TextureType::DISPLACEMENT:
                name = "texture_displacement";
                number = std::to_string(displacementNr++);
                break;
            case TextureType::NONE:
            case TextureType::ICON:
            case TextureType::HEIGHT:
                // todo: implement something
                break;
            }
            shader.setInt(name + number, i);
            glBindTexture(GL_TEXTURE_2D, currentText->ID);
        }
    }

    // draw mesh
    glBindVertexArray(VAO);
    if (hasIndices)
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); // NOLINT
    else
        glDrawArrays(GL_TRIANGLES, 0, verticies.size()); // NOLINT

    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticies.size() * sizeof(Vertex), &verticies[0], GL_STATIC_DRAW);

    if (hasIndices) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    }

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    if (hasNormals) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    }
    if (hasTexCoords) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, TexCoords)));
    }

    glBindVertexArray(0);
}