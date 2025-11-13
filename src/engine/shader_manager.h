#pragma once

#include <memory>
#include <unordered_map>

class Shader;

class ShaderManager {
public:
    ~ShaderManager()
    {
        _shaders.clear();
    } // delete all shaders
    ShaderManager(ShaderManager&) = delete;
    void operator=(const ShaderManager&) = delete;

    // returns instance of shader manager
    static ShaderManager* instance();

    // binds shader with name, returns false if shader doesn't exists
    [[nodiscard]] bool bind(const std::string& name);

    // unbinds shader
    void unbind();

    // reloads shader with name
    void reloadShader(const std::string& name);
    // reloads all shaders
    void reloadAllShaders();

    // todo: add return value bool
    // adds shader with given path and name
    void addShader(const std::string& name, const std::string& vertex_path, const std::string& fragment_path);
    // todo: add return value bool
    // adds shader with given path and name
    void addShader(const std::string& name, const std::string& vertex_path, const std::string& geometry_path, std::string const& fragment_path);

    // returns shade pointer with given name
    [[nodiscard]] Shader* getShader(const std::string& name);

    // returns list of shaders
    std::unordered_map<std::string, std::unique_ptr<Shader>>& getShaderList() { return _shaders; }
    void handleEvents(float dt);

private:
    ShaderManager() = default;
    [[nodiscard]] bool isShaderExist(const std::string& name) { return _shaders.find(name) != _shaders.end(); }
    static ShaderManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
};
inline ShaderManager* ShaderManager::_instance = nullptr;
