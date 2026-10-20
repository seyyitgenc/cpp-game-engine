#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Camera/Camera.hpp"
#include "Camera/CameraController.hpp"
#include "Event/EventBus.hpp"
#include "Platform/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Types/Mesh.hpp"
#include "Scene/Scene.hpp"
#include "UI/ImGuiLayer.hpp"

namespace GNC {

// App
// Owns everything and runs the main loop. Each frame is three traversals, in
// order: event (input and queued events are delivered), update (the world
// moves on), render (the UI is built, the frame drawn and presented).
//
// Members are declared in dependency order and C++ destroys them in reverse,
// which is exactly the teardown needed: listeners and the scene first, then
// ImGui and the renderer (each after waiting for the GPU), then GPU resources,
// the device, the window, and the event bus the window's callbacks enqueue
// onto last of all.
class App {
public:
    // Longest step a single frame may advance the simulation by. After a stall
    // (dragging the window, a breakpoint) one huge step would teleport the
    // camera and everything that moves.
    static constexpr float MaxDeltaTime = 0.1f;

private:
    EventBus eventBus;
    Window window;
    VulkanContext vulkan;

    // Command buffers for load time uploads (mesh buffers, later textures).
    vk::raii::CommandPool uploadPool = nullptr;
    ResourceManager resources;

    Camera camera;
    CameraController cameraController;
    Renderer renderer;
    // After the renderer: its pipeline is built for the swapchain format.
    ImGuiLayer gui;

    Scene scene;

    // Set by the WindowResizeEvent listener, acted on by the render
    // traversal: the listener runs at Flush(), and rebuilding the swapchain
    // belongs at a known point of the frame, not inside a listener.
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
    ResourceContext MakeResourceContext() const;
    void SubscribeToEvents();

    Mesh* LoadMesh(const std::string& id, MeshData data);
    void BuildScene();
    void BuildGui();
};

} // namespace GNC
