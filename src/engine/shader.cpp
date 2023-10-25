#include "shader.h"

//! input is like tihs vertex shader, fragment shader
Shader::Shader(const char *vertexPath, const char *fragmentPath) : Shader(vertexPath, nullptr, fragmentPath){}

//! input is like tihs : vertex shader, geometry shader, fragemnt shader
Shader::Shader(const char *vertexPath,  const char *geometryPath , const char *fragmentPath )
{
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);
    ID = glCreateProgram();
    compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
    if (geometryPath != nullptr)
    {
        std::string geometryCode = readFile(geometryPath);
        compileShader(geometryCode.c_str(),GL_GEOMETRY_SHADER);
    }
}

std::string Shader::readFile(const char *path)
{
    std::string shaderCode;
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        shaderFile.open(path);
        std::stringstream shaderStream; 
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        shaderCode = shaderStream.str();
        
    }
    catch(const std::ifstream::failure& e)
    {
        std::cerr << "ERROR::SHADER::FILE NOT_SUCCEFULLY_READ shader file is located at >> " << path <<  '\n';
    }
    return shaderCode;
}

void Shader::compileShader(const char *code, GLenum type)
{
    GLuint shader;
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);
    checkCompileErrors(shader,"COMPUTE");
    glAttachShader(ID, shader);
    glLinkProgram(ID);
    
    checkCompileErrors(ID, "PROGRAM");
    glDeleteShader(shader);
}

void Shader::checkCompileErrors(GLuint shader, std::string type)
{
    int success;
    char infoLog[512];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::COMPILATION_FAILED " << infoLog << std::endl;
        }        
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::LINKING_FAILED " << infoLog << std::endl;
        }        
    }
    
}

void Shader::setBool(const std::string &name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID,name.c_str()), value);
}

void Shader::setInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(ID,name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const
{
    glUniform1f(glGetUniformLocation(ID,name.c_str()), value);
}

void Shader::setVec2(const std::string &name, glm::vec2 value) const
{
    glUniform2fv(glGetUniformLocation(ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string &name, glm::vec3 value) const
{
    glUniform3fv(glGetUniformLocation(ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string &name, glm::vec4 value) const
{
    glUniform4fv(glGetUniformLocation(ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, glm::mat4 value) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID,name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::bind()
{
    glUseProgram(ID);
}
void Shader::unbind()
{
    glUseProgram(0);
}