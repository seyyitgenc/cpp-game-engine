#include "shader_manager.h"

ShaderManager* ShaderManager::_instance = nullptr;

// void ShaderManager::bind_shader(const std::string& name)
// {
//     _shaders[name]->bind();
// }

// void ShaderManager::unbind_shader(const std::string& name)
// {
//     _shaders[name]->unbind();
// }

// void ShaderManager::reload_shader(const std::string& name)
// {
//     // _shaders[name]->unbind();
//     // _shaders[name]->reload(); // TODO : this is must
// }

void ShaderManager::add_shader(const std::string& name, const char* vertex_path, const char* fragment_path)
{
    Shader *shader{new Shader(vertex_path, fragment_path)};
    std::unique_ptr<Shader> uniquePtr{shader};
    _shaders[name] = std::move(uniquePtr);
}

void ShaderManager::add_shader(const std::string& name, const char* vertex_path, const char* geometry_path,const char* fragment_path)
{
    Shader *shader{new Shader(vertex_path, geometry_path, fragment_path)};
    std::unique_ptr<Shader> uniquePtr{shader};
    _shaders[name] = std::move(uniquePtr);
    //? can add Shader return type for this function
}

Shader& ShaderManager::get_shader(const std::string& name)
{
    return *_shaders[name];
}