#pragma once
#include <glad/glad.h>
#include "shader.h"

class Object
{
protected:
    unsigned int VAO,VBO,EBO;
public:
    Object(){initTriangle();}
    void initTriangle(){
        float vertices[] = {
            // first triangle
            -0.9f, -0.5f, 0.0f,  // left 
            -0.0f, -0.5f, 0.0f,  // right
            -0.45f, 0.5f, 0.0f,  // top 
            // second triangle
            0.0f, -0.5f, 0.0f,  // left
            0.9f, -0.5f, 0.0f,  // right
            0.45f, 0.5f, 0.0f   // top 
        }; 
    
        // unsigned int VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
    
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 
    }
    void draw(Shader &shader){
        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES,0,6);
    }
    };
