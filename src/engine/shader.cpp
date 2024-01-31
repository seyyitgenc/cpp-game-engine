#include "shader.h"
#include "util/log.h"

//! input is like tihs vertex shader, fragment shader
Shader::Shader(const std::string &vertexPath,  const std::string &fragmentPath) : Shader(vertexPath, nullptr, fragmentPath){}

//! input is like tihs : vertex shader, geometry shader, fragemnt shader
Shader::Shader(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath){
    // todo: reaserc about this
    // note: is it good approach ?
    _vertexPath = vertexPath;
    _fragmentPath = fragmentPath;
    _geometryPath = geometryPath;
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
            CLog::write(CLog::Fatal, "ERROR::SHADER::FILE_COMPILATION_FAILED -> %s\n", infoLog);
        }        
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            CLog::write(CLog::Fatal, "ERROR::SHADER::FILE_LINKING_FAILED -> %s\n", infoLog);
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
        CLog::write(CLog::Fatal, "ERROR::SHADER::FILE_NOT_SUCCEFULLY_READ shader file is located at -> %s\n", path.c_str());
    }
    return shaderCode;
}
void Shader::buildShader(){
    std::string vertexCode = readFile(_vertexPath.c_str());
    std::string fragmentCode = readFile(_fragmentPath.c_str());
    ID = glCreateProgram();

    CLog::write(CLog::Debug, "Shader initialized with ID -> %d\n", ID);
    
    compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    CLog::write(CLog::Debug, "Path of vertex Shader -> %s\n", _vertexPath.c_str());
    
    compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
    CLog::write(CLog::Debug, "Path of fragment Shader -> %s\n", _fragmentPath.c_str());
    if (!_geometryPath.empty())
    {
        std::string geometryCode = readFile(_geometryPath.c_str());
        compileShader(geometryCode.c_str(),GL_GEOMETRY_SHADER);
        CLog::write(CLog::Debug, "Path of geometry Shader -> %s\n", _geometryPath.c_str());
    }
    CLog::write(CLog::Debug, "-----------------------------------------------------\n");
}

void Shader::reload(){
    CLog::write(CLog::Debug, "Reloading Shader ...\n");
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

void Shader::bind(){
    glUseProgram(ID);
}
void Shader::unbind(){
    glUseProgram(0);
}