#pragma once

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
#include "UI/ImGuiLayer.hpp"
#include "UI/Selection.hpp"

namespace GNC {

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

    // After the renderer: its pipeline is built for the swapchain format.
    // After the scene, componentTypes and selection: its panels use them.
    ImGuiLayer gui;

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

    Mesh* LoadMesh(const std::string& id, MeshData data);
    void RegisterComponents();
    void BuildScene();
    void BuildGui();
};

} // namespace GNC
