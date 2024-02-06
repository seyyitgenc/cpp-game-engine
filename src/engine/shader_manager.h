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

    void bind_shader(const std::string &name);
    void unbind_shader(const std::string &name);
    void reload_shader(const std::string &name);
    void reload_shaders();
    void add_shader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path);
    void add_shader(const std::string &name, const std::string &vertex_path, const std::string &geometry_path,std::string const &fragment_path);
    Shader& get_shader(const std::string& name);
    std::unordered_map<std::string, std::unique_ptr<Shader>> &get_shader_list(){return _shaders;}
protected:
    ShaderManager() = default;
    static ShaderManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
};
