#pragma once

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


class Shader
{
public:
    Shader(const char *vertexPath, const char *geometryPath, const char *fragmentPath = nullptr);
    // utiliy uniforrm functions
    void setBool(const std::string &name, bool value) const;
   
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
   
    void setVec2(const std::string &name, glm::vec2 value) const;
    void setVec3(const std::string &name, glm::vec3 value) const;
    void setVec4(const std::string &name, glm::vec4 value) const;

    void setMat4(const std::string &name, glm::mat4 value) const;
private:
    // function that return read code from file 
    // ----------------------------------------
    std::string readFile(const char *path);
    void compileShader(const char * code, GLenum type);
    void checkCompileErrors(GLuint shader, std::string type);
public:
    void use();
    unsigned int ID;
};
