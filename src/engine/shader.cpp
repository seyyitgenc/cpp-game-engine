#include "shader.h"

//! input is like tihs vertex shader, fragment shader
Shader::Shader(const std::string &vertexPath,  const std::string &fragmentPath) : Shader(vertexPath, "", fragmentPath){}

//! input is like tihs : vertex shader, geometry shader, fragemnt shader
Shader::Shader(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath) : _vertexPath(vertexPath),  _fragmentPath(fragmentPath) , _geometryPath(geometryPath){
    buildShader();   
}

void checkCompileErrors(GLuint shader, std::string type){
    int success;
    char infoLog[512];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            Log::write(Log::Fatal, LIGHT_RED_TEXT("FATAL::SHADER::FILE_COMPILATION_FAILED"), YELLOW_TEXT(" -> "), YELLOW_TEXT(infoLog) , "\n");
        }        
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            Log::write(Log::Fatal, LIGHT_RED_TEXT("FATAL::SHADER::FILE_LINKING_FAILED"), YELLOW_TEXT( " -> "), YELLOW_TEXT(infoLog), "\n");
        }
    }
    
}
void Shader::compileShader(const char* code, GLenum type){
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

std::string Shader::readFile(const std::string &path){
    std::string shaderCode;
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
        shaderFile.open(path);
        std::stringstream shaderStream; 
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        shaderCode = shaderStream.str();
    }
    catch(const std::ifstream::failure& e){
        Log::write(Log::Fatal, LIGHT_RED_TEXT("FATAL::SHADER::FILE_NOT_SUCCEFULLY_READ "), YELLOW_TEXT("shader file is located at -> "), YELLOW_TEXT(path), "\n");
    }
    return shaderCode;
}
void Shader::buildShader(){
    std::string vertexCode = readFile(_vertexPath.c_str());
    std::string fragmentCode = readFile(_fragmentPath.c_str());
    ID = glCreateProgram();

    Log::write(Log::Debug, LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "), YELLOW_TEXT("Shader initialized with ID -> "), ID, "\n");
    
    compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    Log::write(Log::Debug, LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "), YELLOW_TEXT("Path of vertex Shader -> "), YELLOW_TEXT(_vertexPath), "\n");
    
    compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
    Log::write(Log::Debug, LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "), YELLOW_TEXT("of fragment Shader -> "), YELLOW_TEXT(_fragmentPath), "\n");
    if (!_geometryPath.empty())
    {
        std::string geometryCode = readFile(_geometryPath.c_str());
        compileShader(geometryCode.c_str(),GL_GEOMETRY_SHADER);
        Log::write(Log::Debug, LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "), YELLOW_TEXT("Path of geometry Shader -> "), YELLOW_TEXT(_geometryPath), "\n");
    }
    Log::write(Log::Debug, BLUE_TEXT("-----------------------------------------------------\n"));
}

void Shader::reload(){
    Log::write(Log::Debug, LIGHT_MAGENTA_TEXT("DEBUG::SHADER::RELOAD Reloading Shader ...\n"));
    glDeleteProgram(ID);
    buildShader();
}
void Shader::setBool(const std::string &name, bool value) const{
    glUniform1i(glGetUniformLocation(ID,name.c_str()), value);
}

void Shader::setInt(const std::string &name, int value) const{
    glUniform1i(glGetUniformLocation(ID,name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const{
    glUniform1f(glGetUniformLocation(ID,name.c_str()), value);
}

void Shader::setVec2(const std::string &name, glm::vec2 value) const{
    glUniform2fv(glGetUniformLocation(ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string &name, glm::vec3 value) const{
    glUniform3fv(glGetUniformLocation(ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string &name, glm::vec4 value) const{
    glUniform4fv(glGetUniformLocation(ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, glm::mat4 value) const{
    glUniformMatrix4fv(glGetUniformLocation(ID,name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}