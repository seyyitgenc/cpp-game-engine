#include "camera_manager.h"

#include "GLFW/glfw3.h"

CameraManager* CameraManager::_instance = nullptr;

CameraManager::~CameraManager()
{
    _cameras.clear();
};

[[nodiscard]] CameraManager* CameraManager::instance()
{
    if (_instance == nullptr) {
        _instance = new CameraManager();
        Log::write(
            Log::Info,
            GREEN_TEXT("INFO::CAMERA_MANAGER_GET_INSTANCE Camera Manager with mem address -> "),
            &_instance,
            "\n");
    }
    return _instance;
}

// adds camera with given name and spec
Camera* CameraManager::addCamera(const std::string& name, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
{
    if (isCameraExist(name)) {
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::CAMERA_MANAGER::ADD_CAMERA You tried to add camera that already exist. Key you provided is -> "),
            YELLOW_TEXT(name),
            "\n");
        return nullptr;
    }
    _cameras[name] = std::make_unique<Camera>(position, up, yaw, pitch);
    return _cameras[name].get();
}

Camera* CameraManager::getCamera(const std::string& name)
{
    if (!isCameraExist(name)) {
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::CAMERA_MANAGER::GET_CAMERA You tried to get camera that doesn't exist. Key you provided is -> "),
            YELLOW_TEXT(name),
            "\n");
        return nullptr;
    }
    return _cameras[name].get();
}

void CameraManager::setNextCamera()
{
    auto it = std::find_if(_cameras.begin(), _cameras.end(), [&](const auto& pair) { return pair.second.get() == activeCamera; });
    it++;
    if (it != _cameras.end()) {
        activeCamera = it->second.get();
        Mouse::setFirstMouse(true);
        Log::write(
            Log::Info,
            LIGHT_CYAN_TEXT("INFO::CAMERA_MANAGER::SET_NEXT_CAMERA Switched camera to -> "),
            YELLOW_TEXT(it->first),
            "\n");
    } else {
        Log::write(
            Log::Warning,
            LIGHT_RED_TEXT("WARNING::CAMERA_MANAGER::SET_NEXT_CAMERA There is no camera on right\n"));
    }
}
void CameraManager::setPrevCamera()
{
    auto it = std::find_if(_cameras.begin(), _cameras.end(), [&](const auto& pair) { return pair.second.get() == activeCamera; });
    if (it != _cameras.begin()) {
        it--;
        activeCamera = it->second.get();
        Mouse::setFirstMouse(true);
        Log::write(
            Log::Info,
            LIGHT_CYAN_TEXT("INFO::CAMERA_MANAGER::SET_PREV_CAMERA Switched camera to -> "),
            YELLOW_TEXT(it->first),
            "\n");
    } else {
        Log::write(
            Log::Warning,
            LIGHT_RED_TEXT("WARNING::CAMERA_MANAGER::SET_PREV_CAMERA There is no camera on left\n"));
    }
}

void CameraManager::handleEvents(float dt)
{
    GNC_UNUSED(dt);

    if (Keyboard::keyWentDown(GLFW_KEY_LEFT)) {
        setPrevCamera();
    }
    if (Keyboard::keyWentDown(GLFW_KEY_RIGHT)) {
        setNextCamera();
    }
}