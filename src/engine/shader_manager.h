#pragma once
#include <unordered_map>
#include "shader.h"
#include <memory>


// FIXME : this solutuion is not memory friendly maybe ???
class ShaderManager
{
public:
    ~ShaderManager(){
        // maybe later on i will use shader pointer to hold the shader
        _shaders.clear(); // temporaray solution
    } // delete all shaders
    ShaderManager(ShaderManager&) = delete;
    void operator=(const ShaderManager&) = delete;

    static ShaderManager *getInstance(){ // singleton
        if (_instance == nullptr){
            std::cout << "Shader Manager initialized" << std::endl;
            _instance = new ShaderManager();
        }
        return _instance;
    }

    // void bind_shader(const std::string& name);
    // void unbind_shader(const std::string& name);
    // void reload_shader(const std::string& name);
    void add_shader(const std::string &name, const char* vertex_path, const char* fragment_path);
    void add_shader(const std::string &name, const char* vertex_path, const char* geometry_path,const char* fragment_path);
    Shader& get_shader(const std::string& name);

protected:
    ShaderManager() = default;
    static ShaderManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
};
