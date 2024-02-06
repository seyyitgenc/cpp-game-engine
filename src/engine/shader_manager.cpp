#include "shader_manager.h"
#include "util/log.h"
ShaderManager* ShaderManager::_instance = nullptr;

void ShaderManager::bind_shader(const std::string& name){
    _shaders[name]->bind();
}

void ShaderManager::unbind_shader(const std::string& name){
    _shaders[name]->unbind();
}

void ShaderManager::reload_shader(const std::string& name){
    auto found = _shaders.find(name);
    if (found != _shaders.end())
        _shaders[name]->reload();
    else
        CLog::write(CLog::Fatal, "ERROR::ShaderNotFound with name -> %s\n",name.c_str());
}

void ShaderManager::reload_shaders(){
    for (auto &&i : _shaders)
    {
        i.second->reload();
    }
}

void ShaderManager::add_shader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path){
    add_shader(name,vertex_path, "", fragment_path);
}


void ShaderManager::add_shader(const std::string &name, const std::string &vertex_path, const std::string &geometry_path, const std::string &fragment_path){
    auto found = _shaders.find(name);
    if (found == _shaders.end())
    {
        Shader *shader{new Shader(vertex_path, geometry_path, fragment_path)};
        std::unique_ptr<Shader> uniquePtr{shader};
        _shaders[name] = std::move(uniquePtr);
    }
    //? can add Shader return type for this function
}

Shader& ShaderManager::get_shader(const std::string& name){
    return *_shaders[name];
}