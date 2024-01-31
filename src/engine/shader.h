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
    Shader() = default;
    Shader(const std::string &vertexPath, const std::string &fragmentPath);
    Shader(const std::string &vertexPath, const std::string &geometrypath, const std::string &fragmentPath);
    // utiliy uniforrm functions
    void setBool(const std::string &name, bool value) const;
   
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
   
    void setVec2(const std::string &name, glm::vec2 value) const;
    void setVec3(const std::string &name, glm::vec3 value) const;
    void setVec4(const std::string &name, glm::vec4 value) const;

    void setMat4(const std::string &name, glm::mat4 value) const;
    void reload();
private:
    std::string readFile(const std::string &path);
    void compileShader(const char*  code, GLenum type);
    void buildShader();
public:
    std::string _vertexPath;
    std::string _fragmentPath;
    std::string _geometryPath;
    // FIXME : later on i need to fix this access modifier i don't want user to access these
    void bind();
    void unbind();
    unsigned int ID;
};
