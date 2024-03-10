#pragma once

#include "util/log.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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
    // template<typename T>
    // T getAttribute(const std::string &name){
    //     return static_cast<T>(getGLSLType<T>(name));
    // }
private:
    void initAttributeList(){
        GLint numUniforms;
        glGetProgramiv(_info.ID, GL_ACTIVE_UNIFORMS, &numUniforms);
        for(GLint i = 0; i < numUniforms; i++)
        {
            char buffer[128];
            GLsizei length = 0;
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(_info.ID, i, sizeof(buffer), &length, &size, &type, buffer);
            attributeList.push_back(std::make_pair(buffer,type));
        }   
    }
public:  // uniforrm setter functions
    std::vector<std::pair<std::string, GLenum>> attributeList;
    void setBool(const std::string &name, bool value) const;
   
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
   
    void setVec2(const std::string &name, glm::vec2 value) const;
    void setVec3(const std::string &name, glm::vec3 value) const;
    void setVec4(const std::string &name, glm::vec4 value) const;

    void setMat4(const std::string &name, glm::mat4 value) const;
public: // uniform getter functions
    bool getBool(const std::string &name);
    
    int getInt(const std::string &name);
    float getFloat(const std::string &name);
    
    glm::vec2 getVec2(const std::string &name);
    glm::vec3 getVec3(const std::string &name);
    glm::vec4 getVec4(const std::string &name);

    glm::mat4 getMat4(const std::string &name);
public:
    //  
    void reload();
    void use();
private:
    template<typename T>
    std::string getGLSLType(GLenum type){
        switch(type) {
            case GL_FLOAT_VEC2: return "vec2";
            case GL_FLOAT_VEC3: return "vec3";
            case GL_FLOAT_VEC4: return "vec4";
            case GL_FLOAT:      return "float";
            case GL_BOOL:       return "bool";
            case GL_INT:        return "int";
            // case GL_INT_VEC2: return "ivec2";
            // case GL_INT_VEC3: return "ivec3";
            // case GL_INT_VEC4: return "ivec4";
            // case GL_BOOL_VEC2: return "bvec2";
            // case GL_BOOL_VEC3: return "bvec3";
            // case GL_BOOL_VEC4: return "bvec4";
            // case GL_FLOAT_MAT2: return "mat2";
            // case GL_FLOAT_MAT3: return "mat3";
            // case GL_FLOAT_MAT4: return "mat4";
            // case GL_SAMPLER_2D: return "sampler2D";
            // case GL_SAMPLER_CUBE: return "samplerCube";
            // add more types as needed
            default: return "unknown";
        }
    }
    std::string readFile(const std::string &path);
    void compileShader(const char*  code, GLenum type);
    void buildShader();
private:
    // TODO: add descripton
    // std::string _description;
    // FIXME : later on i need to fix this access modifier i don't want user to access these
    ShaderInfo _info;
};
