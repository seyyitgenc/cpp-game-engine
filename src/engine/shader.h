#pragma once

#include "util/log.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

struct ShaderInfo{
    std::string vertexPath;
    std::string fragmentPath;
    std::string geometryPath;
    std::string description;
    GLuint ID = -1;
    void drawInfo(){
        if (ImGui::BeginTabItem("Details")){
            ImGui::Text("Shader ID : %d", ID);
            ImGui::Text("Shader Vertex Path   : %s", vertexPath.c_str());
            ImGui::Text("Shader Fragment Path : %s", fragmentPath.c_str());
            ImGui::Text("Shader Geometry Path : %s", geometryPath.c_str());
            static float test[3] = {0,1,1};
            ImGui::SliderFloat3("testing",test,0,100);
            ImGui::TextWrapped("Description : %s", description.c_str());
            ImGui::EndTabItem();
        }
    }
};

class Shader
{
public:
    Shader(const std::string &vertexPath, const std::string &fragmentPath);
    Shader(const std::string &vertexPath, const std::string &geometrypath, const std::string &fragmentPath);
    ShaderInfo &getShaderInfo(){return _info;}

public:  // uniforrm setter functions
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
    std::string readFile(const std::string &path);
    bool compileShader(const char*  code, GLuint ID, GLenum type);
    bool buildShader();
private:
    // TODO: add descripton
    // std::string _description;
    // FIXME : later on i need to fix this access modifier i don't want user to access these
    ShaderInfo _info;
};
