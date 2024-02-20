#pragma once 

#include <memory>
#include <unordered_map>

#include "camera.h"
#include "util/log.hpp"

class CameraManager
{
public:
    ~CameraManager(){
        _cameras.clear();
    };
    CameraManager(CameraManager&) = delete;
    void operator=(const CameraManager&) = delete;

    [[nodiscard]] static CameraManager *getInstance(){
        if (_instance == nullptr){
            _instance = new CameraManager();
            Log::write(
                Log::Info,
                GREEN_TEXT("INFO::CAMERA_MANAGER_GET_INSTANCE "),
                YELLOW_TEXT("Camera Manager with mem address -> '"),&_instance,YELLOW_TEXT("'\n"));
        }
        return _instance;
    }

    void addCamera(const std::string& name, glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f), glm::vec3 up = glm::vec3(0.0f,1.0f,0.0f), float yaw = YAW, float pitch = PITCH){
        if (isCameraExist(name))
        {
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::CAMERA_MANAGER::ADD_CAMERA "),
                YELLOW_TEXT("You tried to add camera that already exist. Key you provided is -> '"),
                YELLOW_TEXT(name),"'\n");
            return;
        }
        _cameras[name] = std::make_unique<Camera>(Camera(position, up, yaw, pitch));
    }
    // this function may return nullptr.
    // handling this is users responsibility
    [[nodiscard]] Camera* getCamera(const std::string& name){
        if (!isCameraExist(name))
        {
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::CAMERA_MANAGER::GET_CAMERA"),
                YELLOW_TEXT("You tried to get camera that doesn't exist. Key you provided is -> '"),
                YELLOW_TEXT(name),"'\n");
                return nullptr;
        }
        return _cameras[name].get();
    }
    std::unordered_map<std::string,std::unique_ptr<Camera>> &getCameraList(){
        return _cameras;
    }
private:
    [[nodiscard]] bool isCameraExist(const std::string& name){
    return _cameras.find(name) != _cameras.end();
    }
    CameraManager() = default;
    static CameraManager* _instance;
    std::unordered_map<std::string, std::unique_ptr<Camera>> _cameras;
};
inline CameraManager* CameraManager::_instance = nullptr;
