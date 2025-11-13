#include "shader_manager.h"

#include "io/keyboard.h"
#include "shader.h"

#include "util/helpers.hpp"

#include "glad/glad.h"

#include <GLFW/glfw3.h>

// returns instance of shader manager
ShaderManager* ShaderManager::instance()
{ // singleton
    if (_instance == nullptr) {
        _instance = new ShaderManager();
        Log::write(
            Log::Info,
            GREEN_TEXT("INFO::SHADER_MANAGER::GET_INSTANCE Shader Manager Initialized with mem address -> "),
            &_instance,
            "\n");
    }
    return _instance;
}

// binds shader with name, returns false if shader doesn't exists
[[nodiscard]] bool ShaderManager::bind(const std::string& name)
{
    if (!isShaderExist(name)) {
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::SHADER_MANAGER::BIND Shader with name : "),
            YELLOW_TEXT(name),
            LIGHT_RED_TEXT(" doesn't exist returning false\n"));
        return false;
    }
    _shaders[name]->bind();
    return true;
}

// unbinds shader
void unbind() { glUseProgram(0); }

// reloads shader with name
void ShaderManager::reloadShader(const std::string& name)
{
    auto found = _shaders.find(name);
    if (found != _shaders.end())
        _shaders[name]->reload();
    else
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::SHADER_MANAGER::RELOAD_SHADER Shader not found with name -> "),
            YELLOW_TEXT(name), "\n");
};

// reloads all shaders
void ShaderManager::reloadAllShaders()
{
    for (auto&& i : _shaders)
        i.second->reload();
};

// todo: add return value bool
// adds shader with given path and name
void ShaderManager::addShader(const std::string& name, const std::string& vertex_path, const std::string& fragment_path)
{
    addShader(name, vertex_path, "", fragment_path);
};

// todo: add return value bool
// adds shader with given path and name
void ShaderManager::addShader(const std::string& name, const std::string& vertex_path, const std::string& geometry_path, std::string const& fragment_path)
{
    auto found = _shaders.find(name);
    if (found == _shaders.end())
        _shaders[name] = std::make_unique<Shader>(vertex_path, geometry_path, fragment_path);
};

// returns shade pointer with given name
Shader* ShaderManager::getShader(const std::string& name)
{
    if (!isShaderExist(name)) {
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::SHADER_MANAGER::GET_SHADER Shader with name : "),
            YELLOW_TEXT(name),
            LIGHT_RED_TEXT(" doesn't exist returning -> nullptr\n"));
        return nullptr;
    }

    return _shaders[name].get();
};

void ShaderManager::handleEvents(float dt)
{
    GNC_UNUSED(dt);

    if (Keyboard::keyWentDown(GLFW_KEY_R)) {
        reloadAllShaders();
    }
}
