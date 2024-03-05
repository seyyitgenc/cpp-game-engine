#pragma once

#include "util/log.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

struct ShaderInfo{
    std::string vertexPath;
    std::string fragmentPath;
    std::string geometryPath;
    std::string description;
    GLuint ID;
};

class Shader
{
public:
    Shader(const std::string &vertexPath, const std::string &fragmentPath);
    Shader(const std::string &vertexPath, const std::string &geometrypath, const std::string &fragmentPath);
    ShaderInfo &getShaderInfo(){return _info;}
    // utiliy uniforrm functions
    void setBool(const std::string &name, bool value) const;
   
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
   
    void setVec2(const std::string &name, glm::vec2 value) const;
    void setVec3(const std::string &name, glm::vec3 value) const;
    void setVec4(const std::string &name, glm::vec4 value) const;

    void setMat4(const std::string &name, glm::mat4 value) const;
    void reload();
    void use();
private:
    std::string readFile(const std::string &path);
    void compileShader(const char*  code, GLenum type);
    void buildShader();
private:
    // TODO: add descripton
    // std::string _description;
    // FIXME : later on i need to fix this access modifier i don't want user to access these
    ShaderInfo _info;
};
