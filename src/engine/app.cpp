#include "App.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ECS/Component/LightComponent.hpp"
#include "ECS/Component/MeshComponent.hpp"
#include "ECS/Component/OrbitComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "Event/Events.hpp"
#include "Resource/Primitives.hpp"
#include "Utils/Log.hpp"

namespace GNC {

namespace {

constexpr int WindowWidth = 1600;
constexpr int WindowHeight = 900;

// Rotation that points an entity's -Z axis (the way lights shine) along
// direction.
glm::quat FacingDirection(const glm::vec3& direction) {
    return glm::quatLookAt(glm::normalize(direction), glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace

App::App()
    : window(WindowWidth, WindowHeight, "GNC Engine", eventBus),
      vulkan(window),
      uploadPool(CreateUploadPool()),
      resources(MakeResourceContext()),
      cameraController(eventBus, window, camera),
      renderer(vulkan, window.GetFramebufferExtent(), &camera) {
    SubscribeToEvents();
    BuildScene();
}

void App::Run() {
    Log::write(Log::Info, "Controls: hold right mouse to look, WASD to move, E/Q up/down, Shift faster, wheel zooms, Esc quits\n");

    bool swapchainOutOfDate = false;
    auto previousTime = std::chrono::steady_clock::now();

    while (!window.ShouldClose()) {
        window.PollEvents();

        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::min(std::chrono::duration<float>(now - previousTime).count(), MaxDeltaTime);
        previousTime = now;

        // Minimized: there is no surface to render to, and a 0x0 swapchain
        // cannot exist. Sleep until something happens.
        const vk::Extent2D extent = window.GetFramebufferExtent();
        const bool minimized = extent.width == 0 || extent.height == 0;

        // Callbacks enqueue, Flush() delivers: every listener runs here, at
        // one known point of the frame. FrameEvent goes in behind this
        // frame's input, and only for frames that will be drawn.
        if (!minimized) eventBus.Enqueue<FrameEvent>(deltaTime);
        eventBus.Flush();

        if (minimized) {
            window.WaitEvents();
            continue;
        }

        if (resizePending || swapchainOutOfDate) {
            renderer.Resize(extent);
            resizePending = false;
        }

        cameraController.Update(deltaTime);
        scene.Update(deltaTime);

        swapchainOutOfDate = !renderer.Render(scene.GetEntities());
    }
}

vk::raii::CommandPool App::CreateUploadPool() const {
    // eTransient: its command buffers are short lived, one per upload.
    return vk::raii::CommandPool(vulkan.GetDevice(), vk::CommandPoolCreateInfo {
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = vulkan.GetQueueFamilyIndex()
    });
}

ResourceContext App::MakeResourceContext() const {
    // Uploads go through the one queue that does everything; see VulkanContext.
    return ResourceContext {
        .device = &vulkan.GetDevice(),
        .physicalDevice = &vulkan.GetPhysicalDevice(),
        .transferQueue = &vulkan.GetQueue(),
        .transferPool = &uploadPool
    };
}

void App::SubscribeToEvents() {
    subscriptions.push_back(eventBus.Subscribe<WindowResizeEvent>([this](WindowResizeEvent&) {
        resizePending = true;
        return false;
    }));

    // Esc only asks to quit; whatever ends up deciding (a "save changes?"
    // prompt, later) listens for the request instead of for the key.
    subscriptions.push_back(eventBus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (e.GetKey() != GLFW_KEY_ESCAPE || e.IsRepeat()) return false;
        eventBus.Enqueue<QuitApplicationEvent>();
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<QuitApplicationEvent>([this](QuitApplicationEvent&) {
        window.Close();
        return true;
    }));
}

Mesh* App::LoadMesh(const std::string& id, MeshData data) {
    ResourceHandle<Mesh> handle = resources.Load<Mesh>(id, std::move(data));
    if (!handle) {
        throw std::runtime_error("App: failed to upload mesh '" + id + "'");
    }
    return handle.Get();
}

// A PBR showcase: a material chart over a ground plane, lit by a dim sun, a
// spot light from above, and three coloured point lights circling the chart.
// Light intensities are in physical units (lux / candela), see LightComponent.
void App::BuildScene() {
    camera.SetPerspective(45.0f, camera.GetAspectRatio(), 0.1f, 100.0f);
    camera.SetPosition({ 0.0f, 1.6f, 7.5f });
    camera.LookAt({ 0.0f, 0.5f, 0.0f });

    Mesh* sphereMesh = LoadMesh("sphere", CreateSphere(1.0f, 64, 128));
    Mesh* planeMesh = LoadMesh("plane", CreatePlane(1.0f));

    // ------
    // Ground
    // ------
    Entity& ground = scene.CreateEntity("Ground");
    auto* groundTransform = ground.AddComponent<TransformComponent>();
    groundTransform->SetPosition({ 0.0f, -0.4f, 0.0f });
    groundTransform->SetScale({ 30.0f, 1.0f, 30.0f });
    ground.AddComponent<MeshComponent>(planeMesh, scene.CreateMaterial({
        .baseColorFactor = { 0.18f, 0.18f, 0.18f, 1.0f },
        .metallicFactor = 0.0f,
        .roughnessFactor = 0.9f
    }));

    // --------------
    // Material Chart
    // --------------
    // Roughness rises from left to right. Bottom row: red plastic, a
    // dielectric. Top row: gold, a metal (base color is gold's measured
    // reflectance). The spheres sit on the ground.
    struct ChartRow {
        const char* name;
        glm::vec3 baseColor;
        float metallic;
        float height;
    };
    const ChartRow rows[] = {
        { "Plastic", { 0.8f, 0.08f, 0.08f }, 0.0f, 0.0f },
        { "Gold", { 1.0f, 0.766f, 0.336f }, 1.0f, 1.1f },
    };
    constexpr int Columns = 7;
    constexpr float Spacing = 1.1f;
    constexpr float SphereRadius = 0.4f;

    for (const ChartRow& row : rows) {
        for (int column = 0; column < Columns; ++column) {
            const float t = static_cast<float>(column) / static_cast<float>(Columns - 1);

            Entity& sphere = scene.CreateEntity(std::string(row.name) + " " + std::to_string(column));
            auto* transform = sphere.AddComponent<TransformComponent>();
            transform->SetPosition({ (static_cast<float>(column) - (Columns - 1) * 0.5f) * Spacing, row.height, 0.0f });
            transform->SetScale(glm::vec3(SphereRadius));
            sphere.AddComponent<MeshComponent>(sphereMesh, scene.CreateMaterial({
                .baseColorFactor = glm::vec4(row.baseColor, 1.0f),
                .metallicFactor = row.metallic,
                .roughnessFactor = glm::mix(0.05f, 1.0f, t)
            }));
        }
    }

    // ---
    // Sun
    // ---
    // Dim and warm, from above and behind the camera's left shoulder.
    Entity& sun = scene.CreateEntity("Sun");
    sun.AddComponent<TransformComponent>()->SetRotation(FacingDirection({ 0.5f, -1.0f, -0.6f }));
    auto* sunLight = sun.AddComponent<LightComponent>(LightType::Directional);
    sunLight->SetColor({ 1.0f, 0.93f, 0.82f });
    sunLight->SetIntensity(1.0f); // lux

    // ----
    // Spot
    // ----
    // Straight down onto the middle of the chart from above and in front.
    const glm::vec3 spotPosition { 0.0f, 5.0f, 3.0f };
    Entity& spot = scene.CreateEntity("Spot Light");
    auto* spotTransform = spot.AddComponent<TransformComponent>();
    spotTransform->SetPosition(spotPosition);
    spotTransform->SetRotation(FacingDirection(glm::vec3(0.0f, 0.5f, 0.0f) - spotPosition));
    auto* spotLight = spot.AddComponent<LightComponent>(LightType::Spot);
    spotLight->SetIntensity(60.0f); // candela
    spotLight->SetRange(15.0f);
    spotLight->SetSpotCone(glm::radians(15.0f), glm::radians(25.0f));

    // ---------------------
    // Orbiting Point Lights
    // ---------------------
    // Each carries a small emissive sphere, so you can see where it is. The
    // light sits inside its own bulb and the bulb's surface faces away from
    // it, so the bulb is not lit by its own light.
    struct Orbiter {
        glm::vec3 color;
        float height;
        float speed; // radians per second
        float phase;
    };
    const Orbiter orbiters[] = {
        { { 1.0f, 0.55f, 0.25f }, 0.6f, 0.5f, 0.0f },
        { { 0.3f, 0.6f, 1.0f }, 1.6f, -0.35f, 2.1f },
        { { 1.0f, 0.3f, 0.8f }, 2.4f, 0.25f, 4.2f },
    };
    constexpr float OrbitRadius = 4.0f;
    constexpr float BulbRadius = 0.08f;

    int index = 0;
    for (const Orbiter& orbiter : orbiters) {
        Entity& entity = scene.CreateEntity("Point Light " + std::to_string(index++));
        entity.AddComponent<TransformComponent>()->SetScale(glm::vec3(BulbRadius));
        entity.AddComponent<MeshComponent>(sphereMesh, scene.CreateMaterial({
            .baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f },
            .metallicFactor = 0.0f,
            .roughnessFactor = 1.0f,
            .emissiveFactor = orbiter.color,
            .emissiveStrength = 8.0f
        }));

        auto* light = entity.AddComponent<LightComponent>(LightType::Point);
        light->SetColor(orbiter.color);
        light->SetIntensity(20.0f); // candela
        light->SetRange(12.0f);

        entity.AddComponent<OrbitComponent>(glm::vec3(0.0f, orbiter.height, 0.0f), OrbitRadius, orbiter.speed, orbiter.phase);
    }
}

} // namespace GNC
