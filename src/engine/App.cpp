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
#include "UI/Icons.hpp"
#include "UI/Panels/DemoWindowPanel.hpp"
#include "UI/Panels/EnginePanel.hpp"
#include "UI/Panels/InspectorPanel.hpp"
#include "UI/Panels/ScenePanel.hpp"
#include "UI/Panels/ViewportPanel.hpp"
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
      renderer(vulkan, window.GetFramebufferExtent(), &camera),
      gui(eventBus, window, vulkan, renderer) {
    SubscribeToEvents();
    RegisterComponents();
    BuildScene();
    BuildGui();
}

void App::Run() {
    Log::write(Log::Info, "Controls: hold right mouse to look, WASD to move, E/Q up/down, Shift faster, wheel zooms, Esc quits\n");

    auto previousTime = std::chrono::steady_clock::now();

    while (!window.ShouldClose()) {
        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::min(std::chrono::duration<float>(now - previousTime).count(), MaxDeltaTime);
        previousTime = now;

        EventTraversal(deltaTime);

        // Minimized: there is no surface to render to, and a 0x0 swapchain
        // cannot exist. Sleep until something happens.
        if (window.IsMinimized()) {
            window.WaitEvents();
            continue;
        }

        UpdateTraversal(deltaTime);
        RenderTraversal();
    }
}

// Every listener runs here, ImGui's first. FrameEvent goes in behind this
// frame's input, and only for frames that will be drawn.
void App::EventTraversal(float deltaTime) {
    window.PollEvents();

    if (!window.IsMinimized()) eventBus.Enqueue<FrameEvent>(deltaTime);
    eventBus.Flush();
}

void App::UpdateTraversal(float deltaTime) {
    cameraController.Update(deltaTime);
    scene.Update(deltaTime);
}

void App::RenderTraversal() {
    if (resizePending || swapchainOutOfDate) {
        renderer.Resize(window.GetFramebufferExtent());
        resizePending = false;
    }

    // The UI first: ImGuiPass records the draw data it leaves, and the
    // Viewport panel sets the scene's size.
    gui.Render();
    swapchainOutOfDate = !renderer.Render(scene.GetEntities());
    // Panels dragged out of the main window draw and present on their own.
    gui.RenderPlatformWindows();
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

    // Esc only asks to quit, so a later "save changes?" prompt can listen for
    // the request.
    subscriptions.push_back(eventBus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (e.GetKey() != GLFW_KEY_ESCAPE || e.IsRepeat()) return false;
        eventBus.Enqueue<QuitApplicationEvent>();
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<QuitApplicationEvent>([this](QuitApplicationEvent&) {
        window.Close();
        return true;
    }));

    // Scene requests from the UI (SceneEvents.hpp).
    subscriptions.push_back(eventBus.Subscribe<CreateEntityEvent>([this](CreateEntityEvent& e) {
        Entity& entity = scene.CreateEntity(scene.MakeUniqueName(e.GetEntityName()));
        entity.AddComponent<TransformComponent>();
        selection.Select(&entity);
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<AddComponentEvent>([this](AddComponentEvent& e) {
        if (const ComponentRegistry::Entry* type = componentTypes.Find(e.GetComponentTypeID())) {
            type->addTo(e.GetEntity());
        }
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<RemoveComponentEvent>([](RemoveComponentEvent& e) {
        e.GetEntity().RemoveComponent(e.GetComponentTypeID());
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

// In the order the Add Component menus list them.
void App::RegisterComponents() {
    componentTypes.Register<TransformComponent>({ .icon = ICON_FA_UP_DOWN_LEFT_RIGHT });

    // A material of its own, so it can be edited. The mesh is looked up when
    // added, so this can run before BuildScene() loads it.
    componentTypes.Register<MeshComponent>({
        .icon = ICON_FA_DRAW_POLYGON,
        .addTo = [this](Entity& entity) {
            Material* material = scene.CreateMaterial({ .metallicFactor = 0.0f, .roughnessFactor = 0.5f });
            entity.AddComponent<MeshComponent>(resources.GetResource<Mesh>("sphere"), material);
        }
    });

    componentTypes.Register<LightComponent>({ .icon = ICON_FA_LIGHTBULB });

    // Around where the entity is now.
    componentTypes.Register<OrbitComponent>({
        .icon = ICON_FA_SATELLITE,
        .addTo = [](Entity& entity) {
            const TransformComponent* transform = entity.GetComponent<TransformComponent>();
            const glm::vec3 center = transform ? transform->GetPosition() : glm::vec3(0.0f);
            entity.AddComponent<OrbitComponent>(center, 0.5f, 1.0f);
        }
    });
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

// Menu entries and panels appear in the order added.
void App::BuildGui() {
    ImGuiMenu& menuBar = gui.GetMenuBar();

    // ----
    // File
    // ----
    // Asks to quit, as Esc does (see SubscribeToEvents()).
    menuBar.GetMenu("File").AddItem("Quit", [this] { eventBus.Enqueue<QuitApplicationEvent>(); }, { .shortcut = "Esc" });

    // ----
    // View
    // ----
    ImGuiMenu& view = menuBar.GetMenu("View");
    view.AddPanelItem(*gui.AddPanel<ViewportPanel>("Viewport", renderer));
    view.AddPanelItem(*gui.AddPanel<ScenePanel>("Scene", scene, componentTypes, selection, eventBus));
    view.AddPanelItem(*gui.AddPanel<InspectorPanel>("Inspector", selection, componentTypes, resources, eventBus));
    view.AddPanelItem(*gui.AddPanel<EnginePanel>("Engine", camera));
    view.AddSeparator();
    view.AddPanelItem(*gui.AddPanel<DemoWindowPanel>("ImGui Demo"));
}

} // namespace GNC
