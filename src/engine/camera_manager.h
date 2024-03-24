#pragma once 

#include "camera.h"
#include "../util/log.hpp"

#include <map>
#include <memory>
#include <algorithm>

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
                GREEN_TEXT("INFO::CAMERA_MANAGER_GET_INSTANCE Camera Manager with mem address -> "),
                &_instance,
                "\n");
        }
        return _instance;
    }

    void addCamera(const std::string& name, glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f), glm::vec3 up = glm::vec3(0.0f,1.0f,0.0f), float yaw = YAW, float pitch = PITCH){
        if (isCameraExist(name))
        {
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::CAMERA_MANAGER::ADD_CAMERA You tried to add camera that already exist. Key you provided is -> "),
                YELLOW_TEXT(name),
                "\n");
            return;
        }
        _cameras[name] = std::make_unique<Camera>(position, up, yaw, pitch);
    }
    // this function may return nullptr.
    // handling this is users responsibility
    [[nodiscard]] Camera* getCamera(const std::string& name){
        if (!isCameraExist(name))
        {
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::CAMERA_MANAGER::GET_CAMERA You tried to get camera that doesn't exist. Key you provided is -> "),
                YELLOW_TEXT(name),
                "\n");
            return nullptr;
        }
        return _cameras[name].get();
    }
    std::map<std::string,std::unique_ptr<Camera>> *getCameraList(){
        return &_cameras;
    }

public:
    // helper functions
    Camera* getActiveCamera(){return activeCamera;}
    Camera* setActiveCamera(Camera *newCam){return activeCamera = newCam;}
    void setNextCamera(){
        auto it = std::find_if(_cameras.begin(),_cameras.end(),[&](const auto& pair){return pair.second.get() == activeCamera;});
        it++;
        if (it != _cameras.end())
        {
            activeCamera = it->second.get();
            activeCamera->setLastMouse(true);
            Log::write(
                Log::Info,
                LIGHT_CYAN_TEXT("INFO::CAMERA_MANAGER::SET_NEXT_CAMERA Switched camera to -> "),
                YELLOW_TEXT(it->first),
                "\n");
        }
        else{
            Log::write(
                Log::Warning,
                LIGHT_RED_TEXT("WARNING::CAMERA_MANAGER::SET_NEXT_CAMERA There is no camera on right\n"));
        }
    }
    void setPrevCamera(){
        auto it = std::find_if(_cameras.begin(),_cameras.end(),[&](const auto& pair){return pair.second.get() == activeCamera;});
        if (it != _cameras.begin())
        {
            it--;
            activeCamera = it->second.get();
            activeCamera->setLastMouse(true);
            Log::write(
                Log::Info,
                LIGHT_CYAN_TEXT("INFO::CAMERA_MANAGER::SET_PREV_CAMERA Switched camera to -> "),
                YELLOW_TEXT(it->first),
                "\n");
        }
        else{
            Log::write(
                Log::Warning,
                LIGHT_RED_TEXT("WARNING::CAMERA_MANAGER::SET_PREV_CAMERA There is no camera on left\n"));
        }
    }
private:
    [[nodiscard]] bool isCameraExist(const std::string& name){
    return _cameras.find(name) != _cameras.end();
    }
    CameraManager() = default;
private:
    std::map<std::string, std::unique_ptr<Camera>> _cameras;
    static CameraManager* _instance;
    Camera* activeCamera;
};
inline CameraManager* CameraManager::_instance = nullptr;