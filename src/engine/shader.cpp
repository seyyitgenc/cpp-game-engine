#include "shader.h"

//! input is like tihs vertex shader, fragment shader
Shader::Shader(const std::string &vertexPath,  const std::string &fragmentPath) : Shader(vertexPath, "", fragmentPath){}

//! input is like tihs : vertex shader, geometry shader, fragemnt shader
Shader::Shader(const std::string &vertexPath, const std::string &geometryPath, const std::string &fragmentPath) : _info{vertexPath, fragmentPath, geometryPath, "DESCRIPTION IS NOT SPECIFIED"}{
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
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::SHADER::FILE_COMPILATION_FAILED"),
                YELLOW_TEXT(" -> "),
                YELLOW_TEXT(infoLog) , "\n");
        }        
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::SHADER::FILE_LINKING_FAILED"),
                YELLOW_TEXT( " -> "),
                YELLOW_TEXT(infoLog), "\n");
        }
    }
    
}
void Shader::compileShader(const char* code, GLenum type){
    GLuint shader;
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);
    checkCompileErrors(shader,"COMPUTE");
    glAttachShader(_info.ID, shader);
    glLinkProgram(_info.ID);
    
    checkCompileErrors(_info.ID, "PROGRAM");
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
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::SHADER::FILE_NOT_SUCCEFULLY_READ "),
            YELLOW_TEXT("shader file is located at -> "),
            YELLOW_TEXT(path), "\n");
    }
    return shaderCode;
}
void Shader::buildShader(){
    std::string vertexCode = readFile(_info.vertexPath.c_str());
    std::string fragmentCode = readFile(_info.fragmentPath.c_str());
    _info.ID = glCreateProgram();

    Log::write(
        Log::Debug,
        LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "),
        YELLOW_TEXT("Shader initialized with _info.ID -> "),_info.ID, "\n");
    
    compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);

    Log::write(
        Log::Debug,
        LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "),
        YELLOW_TEXT("Path of vertex Shader -> "),
        YELLOW_TEXT(_info.vertexPath), "\n");
    
    compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);

    Log::write(
        Log::Debug,
        LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "),
        YELLOW_TEXT("of fragment Shader -> "),
        YELLOW_TEXT(_info.fragmentPath), "\n");

    if (!_info.geometryPath.empty())
    {
        std::string geometryCode = readFile(_info.geometryPath.c_str());
        compileShader(geometryCode.c_str(),GL_GEOMETRY_SHADER);
        Log::write(
            Log::Debug,
            LIGHT_MAGENTA_TEXT("DEBUG::SHADER::BUILD_SHADER "),
            YELLOW_TEXT("Path of geometry Shader -> "),
            YELLOW_TEXT(_info.geometryPath), "\n");
    }
    initAttributeList(); // note: it may crush here be careful
    Log::write(Log::Debug, BLUE_TEXT("-----------------------------------------------------\n"));
}

void Shader::reload(){
    Log::write(
        Log::Debug,
        LIGHT_MAGENTA_TEXT("DEBUG::SHADER::RELOAD Reloading Shader ...\n"));
        
    glDeleteProgram(_info.ID);
    buildShader();
}

void Shader::use(){
    glUseProgram(_info.ID);
}
void Shader::setBool(const std::string &name, bool value) const{
    glUniform1i(glGetUniformLocation(_info.ID,name.c_str()), value);
}

void Shader::setInt(const std::string &name, int value) const{
    glUniform1i(glGetUniformLocation(_info.ID,name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const{
    glUniform1f(glGetUniformLocation(_info.ID,name.c_str()), value);
}

void Shader::setVec2(const std::string &name, glm::vec2 value) const{
    glUniform2fv(glGetUniformLocation(_info.ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string &name, glm::vec3 value) const{
    glUniform3fv(glGetUniformLocation(_info.ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string &name, glm::vec4 value) const{
    glUniform4fv(glGetUniformLocation(_info.ID,name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, glm::mat4 value) const{
    glUniformMatrix4fv(glGetUniformLocation(_info.ID,name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

bool Shader::getBool(const std::string &name){
    GLint value;
    glGetUniformiv(_info.ID, glGetUniformLocation(_info.ID, name.c_str()), &value);
    return static_cast<bool>(value);
};
int Shader::getInt(const std::string &name){
    GLint value;
    glGetUniformiv(_info.ID, glGetUniformLocation(_info.ID, name.c_str()), &value);
    return value;
};
float Shader::getFloat(const std::string &name){
    GLfloat value;
    glGetUniformfv(_info.ID, glGetUniformLocation(_info.ID, name.c_str()), &value);
    return value;
};

glm::vec2 Shader::getVec2(const std::string &name){
    GLfloat value[2];
    glGetUniformfv(_info.ID, glGetUniformLocation(_info.ID, name.c_str()), value);
    return glm::make_vec2(value);
};

glm::vec3 Shader::getVec3(const std::string &name){
    GLfloat value[3];
    glGetUniformfv(_info.ID, glGetUniformLocation(_info.ID, name.c_str()), value);
    return glm::make_vec3(value);
};

glm::vec4 Shader::getVec4(const std::string &name){
    GLfloat value[4];
    glGetUniformfv(_info.ID, glGetUniformLocation(_info.ID, name.c_str()), value);
    return glm::make_vec4(value);
};

glm::mat4 Shader::getMat4(const std::string &name){
    GLfloat value[16];
    glGetUniformfv(_info.ID, glGetUniformLocation(_info.ID, name.c_str()), value);
    return glm::make_mat4(value);
};
