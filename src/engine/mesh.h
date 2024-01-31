#pragma once
#include <vector>
#include "shader.h"

// todo: write down implementation of this clas into .cpp file

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture
{
    unsigned int id;
    std::string path;
    std::string type;
};


class Mesh
{    
public:
    std::vector<Vertex> verticies;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    bool hasIndices;
    // bool hasTextures;
    bool hasNormals;
    bool hasTexCoords;
    unsigned int VAO, VBO, EBO;
public:
    Mesh(std::vector<Vertex> verticies , std::vector<unsigned int> indices, std::vector<Texture> textures,
    bool hasIndices = false, bool hasNormals = false, bool hasTexCoords = false) 
    : hasIndices(hasIndices), hasNormals(hasNormals), hasTexCoords(hasTexCoords){
        this->verticies = verticies;
        this->indices = indices;
        this->textures = textures;
        setupMesh();
    }
    ~Mesh() = default;

    void Draw(Shader& shader){
        if (hasTexCoords)
        {
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            unsigned int normalNr = 1;
            unsigned int depthNr = 1;
            for (unsigned int i = 0; i < textures.size(); i++)
            {
                glActiveTexture(GL_TEXTURE0 + i);
                std::string number;
                std::string name = textures[i].type;
                if (name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if(name == "texture_specular")
                    number = std::to_string(specularNr++);
                shader.setInt(name + number,i);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }
        }
                
        // draw mesh
        glBindVertexArray(VAO);
        if (hasIndices)
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES,0,verticies.size());
            
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    
    }
private:
    void setupMesh(){
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verticies.size() * sizeof(Vertex), &verticies[0], GL_STATIC_DRAW);

        if (hasIndices)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        }

        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),(void*)0);

        if (hasNormals)
        {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1 ,3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        }
        if (hasTexCoords)
        {
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),(void*)(offsetof(Vertex, TexCoords)));
        }
        
        glBindVertexArray(0);
    }
};
