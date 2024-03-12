#pragma once
#include "shader.h"
#include <unordered_map>
#include <memory>

class ShaderManager
{
public:
    ~ShaderManager(){
        _shaders.clear();
    } // delete all shaders
    ShaderManager(ShaderManager&) = delete;
    void operator=(const ShaderManager&) = delete;

    static ShaderManager *getInstance(){ // singleton
        if (_instance == nullptr){
            _instance = new ShaderManager();
            Log::write(
                Log::Info,
                GREEN_TEXT("INFO::SHADER_MANAGER::GET_INSTANCE Shader Manager Initialized with mem address -> "), 
                &_instance, 
                "\n");
        }
        return _instance;
    }

    [[nodiscard]] bool bind(const std::string &name){
        if (!isShaderExist(name)){
            Log::write(
                Log::Fatal, 
                LIGHT_RED_TEXT("FATAL::SHADER_MANAGER::BIND Shader with name : "),
                YELLOW_TEXT(name),
                LIGHT_RED_TEXT(" doesn't exist returning false\n"));
            return false;
        }
        glUseProgram(_shaders[name]->getShaderInfo().ID);
        return true;
    }
    void unbind(){glUseProgram(0);}
    void reloadShader(const std::string &name){    
        auto found = _shaders.find(name);
        if (found != _shaders.end())
            _shaders[name]->reload();
        else
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::SHADER_MANAGER::RELOAD_SHADER Shader not found with name -> "),
            YELLOW_TEXT(name), "\n");
    };
    void reloadAllShaders(){
        for (auto &&i : _shaders)
            i.second->reload();
    };
    void addShader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path){
        addShader(name,vertex_path, "", fragment_path);
    };
    void addShader(const std::string &name, const std::string &vertex_path, const std::string &geometry_path,std::string const &fragment_path){
        auto found = _shaders.find(name);
        if (found == _shaders.end())
            _shaders[name] = std::make_unique<Shader>(Shader(vertex_path, geometry_path, fragment_path));
        //? can add Shader return type for this function
    };
    [[nodiscard]] Shader* getShader(const std::string &name){
        if (!isShaderExist(name))
        {
            Log::write(
                Log::Fatal, 
                LIGHT_RED_TEXT("FATAL::SHADER_MANAGER::GET_SHADER Shader with name : "),
                YELLOW_TEXT(name),
                LIGHT_RED_TEXT(" doesn't exist returning -> nullptr\n"));
            return nullptr;
        }
        
        return _shaders[name].get();
    };
    std::unordered_map<std::string, std::unique_ptr<Shader>> &get_shader_list(){return _shaders;}
private:
    ShaderManager() = default;
    [[nodiscard]] bool isShaderExist(const std::string &name){
        return _shaders.find(name) != _shaders.end();
    }
    static ShaderManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
};
inline ShaderManager* ShaderManager::_instance = nullptr;
