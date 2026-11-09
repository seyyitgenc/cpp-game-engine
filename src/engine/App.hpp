#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Camera/Camera.hpp"
#include "Camera/CameraController.hpp"
#include "ECS/Component/ComponentRegistry.hpp"
#include "Event/EventBus.hpp"
#include "Platform/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/TextureTable.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Types/Mesh.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSettings.hpp"
#include "UI/ImGuiLayer.hpp"
#include "UI/Selection.hpp"

namespace GNC {

class ImGuiMenu;
class SaveScenePanel;

// App
// Owns everything and runs the main loop: an event, an update and a render
// traversal per frame. Members are declared in dependency order, so C++
// destroys them in the order Vulkan needs.
class App {
public:
    // After a stall (a window drag, a breakpoint), one huge step would
    // teleport everything that moves.
    static constexpr float MaxDeltaTime = 0.1f;

private:
    EventBus eventBus;
    Window window;
    VulkanContext vulkan;

    // For load time uploads.
    vk::raii::CommandPool uploadPool = nullptr;
    // Before the resources: textures leave it as they unload.
    TextureTable textureTable;
    ResourceManager resources;

    Camera camera;
    CameraController cameraController;
    Renderer renderer;

    Scene scene;
    // Its entries capture this.
    ComponentRegistry componentTypes;
    // After the scene, which it points into.
    Selection selection;

    // The scene's file; empty for a new scene not saved yet.
    std::filesystem::path scenePath;

    // After the renderer: its pipeline is built for the swapchain format.
    // After the scene, componentTypes and selection: its panels use them.
    ImGuiLayer gui;
    // Owned by gui.
    ImGuiMenu* openSceneMenu = nullptr;
    SaveScenePanel* saveScenePanel = nullptr;

    // Acted on by the render traversal, not inside the listener.
    bool resizePending = false;
    // Set when the last Renderer::Render() found the swapchain out of date.
    bool swapchainOutOfDate = false;

    // App's own listeners. Declared last, so they are dropped first.
    std::vector<EventSubscription> subscriptions;

public:
    App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Returns when the window closes.
    void Run();

private:
    void EventTraversal(float deltaTime);
    void UpdateTraversal(float deltaTime);
    void RenderTraversal();

    vk::raii::CommandPool CreateUploadPool() const;
    ResourceContext MakeResourceContext();
    void SubscribeToEvents();

    void RegisterComponents();
    void BuildGui();

    // ------
    // Scenes
    // ------
    // Replaces the scene. A file that fails to load is logged and leaves
    // the scene as it was.
    bool OpenScene(const std::filesystem::path& path);
    void NewScene(SceneDimension dimension);
    // The file becomes the scene's; a failed save is logged.
    bool SaveScene(const std::filesystem::path& path);
    // Saves to the scene's file, or asks for a name if it has none.
    void SaveOrAskForName();

    void ReplaceScene(Scene&& next, std::filesystem::path path);
    // Fits the editor camera and its controller to the scene's dimension.
    void ResetEditorCamera();
    // The editor camera takes the scene camera's place and projection;
    // false if the scene has none.
    bool MoveCameraToSceneCamera();
    void UpdateSceneMenu();
};

} // namespace GNC
