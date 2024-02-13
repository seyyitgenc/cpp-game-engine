#include "shader_manager.h"
#include "util/log.h"
ShaderManager* ShaderManager::_instance = nullptr;

void ShaderManager::bindShader(const std::string& name){
    _shaders[name]->bind();
}

void ShaderManager::unbindShader(const std::string& name){
    _shaders[name]->unbind();
}

void ShaderManager::reloadShader(const std::string& name){
    auto found = _shaders.find(name);
    if (found != _shaders.end())
        _shaders[name]->reload();
    else
        CLog::write(CLog::Fatal, "ERROR::ShaderNotFound with name -> %s\n",name.c_str());
}

void ShaderManager::reloadAllShaders(){
    for (auto &&i : _shaders)
    {
        i.second->reload();
    }
}

void ShaderManager::addShader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path){
    addShader(name,vertex_path, "", fragment_path);
}


void ShaderManager::addShader(const std::string &name, const std::string &vertex_path, const std::string &geometry_path, const std::string &fragment_path){
    auto found = _shaders.find(name);
    if (found == _shaders.end())
    {
        Shader *shader{new Shader(vertex_path, geometry_path, fragment_path)};
        std::unique_ptr<Shader> uniquePtr{shader};
        _shaders[name] = std::move(uniquePtr);
    }
    //? can add Shader return type for this function
}

Shader& ShaderManager::getShader(const std::string& name){
    return *_shaders[name];
}