#pragma once
#include "shader.h"
#include <unordered_map>
#include <memory>
#include "util/log.h"

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

    void bindShader(const std::string &name){glUseProgram(_shaders[name]->ID);}
    void unbindShader(const std::string &name){glUseProgram(0);}
    void reloadShader(const std::string &name){    
        auto found = _shaders.find(name);
        if (found != _shaders.end())
            _shaders[name]->reload();
        else
        CLog::write(CLog::Fatal, "ERROR::ShaderNotFound with name -> %s\n",name.c_str());
    };
    void reloadAllShaders(){
        for (auto &&i : _shaders)
        {
            i.second->reload();
        }
    };
    void addShader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path){
        addShader(name,vertex_path, "", fragment_path);
    };
    void addShader(const std::string &name, const std::string &vertex_path, const std::string &geometry_path,std::string const &fragment_path){
        auto found = _shaders.find(name);
        if (found == _shaders.end())
        {
            Shader *shader{new Shader(vertex_path, geometry_path, fragment_path)};
            std::unique_ptr<Shader> uniquePtr{shader};
            _shaders[name] = std::move(uniquePtr);
        }
        //? can add Shader return type for this function
    };
    Shader& getShader(const std::string &name){
        return *_shaders[name];
    };
    std::unordered_map<std::string, std::unique_ptr<Shader>> &get_shader_list(){return _shaders;}
private:
    ShaderManager() = default;
    static ShaderManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
};
inline ShaderManager* ShaderManager::_instance = nullptr;
