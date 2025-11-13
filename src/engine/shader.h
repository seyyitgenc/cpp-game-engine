#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct ShaderInfo {
    std::string vertexPath;
    std::string fragmentPath;
    std::string geometryPath;
    std::string description;
    unsigned int ID = UINT32_MAX;

    void drawInfo();
};

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    Shader(const std::string& vertexPath, const std::string& geometrypath, const std::string& fragmentPath);
    ShaderInfo& getShaderInfo() { return _info; }

public: // uniforrm setter functions
    void setBool(const std::string& name, bool value) const;

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;

    void setVec2(const std::string& name, glm::vec2 value) const;
    void setVec3(const std::string& name, glm::vec3 value) const;
    void setVec4(const std::string& name, glm::vec4 value) const;

    void setMat4(const std::string& name, glm::mat4 value) const;

public: // uniform getter functions
    bool getBool(const std::string& name);

    int getInt(const std::string& name);
    float getFloat(const std::string& name);

    glm::vec2 getVec2(const std::string& name);
    glm::vec3 getVec3(const std::string& name);
    glm::vec4 getVec4(const std::string& name);

    glm::mat4 getMat4(const std::string& name);

public:
    //
    void reload();
    void bind();
    void unbind();
    std::string readFile(const std::string& path);
    bool compileShader(const char* code, unsigned int ID, unsigned int type);
    bool buildShader();

private:
    // TODO: add descripton
    // std::string _description;
    // FIXME : later on i need to fix this access modifier i don't want user to access these
    ShaderInfo _info;
};
