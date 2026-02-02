#pragma once

#include "util/helpers.hpp"
#include "util/log.hpp"

#include "camera.h"

#include <algorithm>
#include <map>
#include <memory>

class CameraManager {
public:
    ~CameraManager();

    CameraManager(CameraManager&) = delete;
    void operator=(const CameraManager&) = delete;

    // returns camera manager instance
    [[nodiscard]] static CameraManager* instance();

    // adds camera with given name and spec
    Camera* addCamera(
        const std::string& name,
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW,
        float pitch = PITCH);
    // ! this function may return nullptr.
    // ! handling this is users responsibility
    // returns camera pointer with name
    [[nodiscard]] Camera* getCamera(const std::string& name);

    // returns list of cameras
    std::map<std::string, std::unique_ptr<Camera>>* getCameraList() { return &_cameras; }

public:
    // helper functions
    Camera* getActiveCamera() { return activeCamera; }
    Camera* setActiveCamera(Camera* newCam) { return activeCamera = newCam; }

    void setNextCamera();
    void setPrevCamera();

    void handleEvents(float dt);

private:
    [[nodiscard]] bool isCameraExist(const std::string& name) { return _cameras.find(name) != _cameras.end(); }
    CameraManager() = default;

private:
    std::map<std::string, std::unique_ptr<Camera>> _cameras;
    static CameraManager* _instance;
    Camera* activeCamera;
};