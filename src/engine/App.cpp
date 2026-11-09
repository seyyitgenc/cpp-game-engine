#include "App.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <utility>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ECS/Component/BoundingVolumeComponent.hpp"
#include "ECS/Component/CameraComponent.hpp"
#include "ECS/Component/LightComponent.hpp"
#include "ECS/Component/MeshComponent.hpp"
#include "ECS/Component/OrbitComponent.hpp"
#include "ECS/Component/TransformsComponent.hpp"
#include "Event/Events.hpp"
#include "Resource/Primitives.hpp"
#include "Scene/ModelImporter.hpp"
#include "Scene/SceneSerializer.hpp"
#include "UI/Icons.hpp"
#include "UI/ImGuiMenu.hpp"
#include "UI/Panels/DemoWindowPanel.hpp"
#include "UI/Panels/EnginePanel.hpp"
#include "UI/Panels/InspectorPanel.hpp"
#include "UI/Panels/SaveScenePanel.hpp"
#include "UI/Panels/ScenePanel.hpp"
#include "UI/Panels/ViewportPanel.hpp"
#include "Utils/FileSystem.hpp"
#include "Utils/Log.hpp"

namespace GNC {

namespace {

constexpr int WindowWidth = 1600;
constexpr int WindowHeight = 900;

// In FileSystem::getSceneDirectory(); a new 3D scene if it does not load.
constexpr const char* StartupScene = "Showcase.gncscene";
constexpr const char* SceneExtension = ".gncscene";

// Rotation that points an entity's -Z axis (the way lights shine) along
// direction.
glm::quat FacingDirection(const glm::vec3& direction) {
    return glm::quatLookAt(glm::normalize(direction), glm::vec3(0.0f, 1.0f, 0.0f));
}

// Every file assimp reads among those the models target copied next to the
// executable.
std::vector<std::filesystem::path> FindModels() {
    std::vector<std::filesystem::path> models;
    std::error_code error;
    for (auto it = std::filesystem::recursive_directory_iterator(FileSystem::getPath("models"), error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        if (it->is_regular_file() && IsModelFile(it->path())) models.push_back(it->path());
    }
    std::ranges::sort(models);
    return models;
}

std::vector<std::filesystem::path> FindScenes() {
    std::vector<std::filesystem::path> scenes;
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator(FileSystem::getSceneDirectory(), error);
         !error && it != std::filesystem::directory_iterator(); it.increment(error)) {
        if (it->is_regular_file() && it->path().extension() == SceneExtension) scenes.push_back(it->path());
    }
    std::ranges::sort(scenes);
    return scenes;
}

} // namespace

App::App()
    : window(WindowWidth, WindowHeight, "GNC Engine", eventBus),
      vulkan(window),
      uploadPool(CreateUploadPool()),
      textureTable(MakeResourceContext()),
      resources(MakeResourceContext()),
      cameraController(eventBus, window, camera),
      renderer(vulkan, window.GetFramebufferExtent(), &camera, textureTable),
      gui(eventBus, window, vulkan, renderer) {
    SubscribeToEvents();
    RegisterComponents();
    BuildGui();
    if (!OpenScene(FileSystem::getSceneDirectory() / StartupScene)) NewScene(SceneDimension::ThreeD);
}

void App::Run() {
    Log::write(Log::Info, "Controls: hold right mouse to look and WASD to fly, E/Q up/down, Shift faster, wheel zooms; "
                          "left click selects, W/E/R move/rotate/scale it; Esc quits\n");

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

    renderer.SetAmbientLight(scene.GetEnvironment().ambientColor);
    renderer.SetExposure(scene.GetEnvironment().exposure);
    renderer.SetGridPlane(scene.GetDimension() == SceneDimension::TwoD ? GridPlane::XY : GridPlane::XZ);

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

ResourceContext App::MakeResourceContext() {
    // Uploads go through the one queue that does everything; see VulkanContext.
    return ResourceContext {
        .device = &vulkan.GetDevice(),
        .physicalDevice = &vulkan.GetPhysicalDevice(),
        .transferQueue = &vulkan.GetQueue(),
        .transferPool = &uploadPool,
        .textures = &textureTable
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

    // Through the same request as the menus, delivered at the next Flush().
    subscriptions.push_back(eventBus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (e.GetKey() != GLFW_KEY_DELETE || e.IsRepeat() || !selection.GetEntity()) return false;
        eventBus.Enqueue<DestroyEntityEvent>(*selection.GetEntity());
        return true;
    }));

    // As File > Save Scene.
    subscriptions.push_back(eventBus.Subscribe<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (e.GetKey() != GLFW_KEY_S || !(e.GetMods() & GLFW_MOD_CONTROL) || e.IsRepeat()) return false;
        SaveOrAskForName();
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<QuitApplicationEvent>([this](QuitApplicationEvent&) {
        window.Close();
        return true;
    }));

    // Scene requests from the UI (SceneEvents.hpp).
    subscriptions.push_back(eventBus.Subscribe<NewSceneEvent>([this](NewSceneEvent& e) {
        NewScene(e.GetDimension());
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<OpenSceneEvent>([this](OpenSceneEvent& e) {
        OpenScene(e.GetPath());
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<SaveSceneEvent>([this](SaveSceneEvent& e) {
        SaveScene(e.GetPath());
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<CreateEntityEvent>([this](CreateEntityEvent& e) {
        Entity* parent = e.GetParent();
        if (parent && !scene.Contains(parent)) return true;

        Entity& entity = scene.CreateEntity(scene.MakeUniqueName(e.GetEntityName(), parent), parent);
        entity.AddComponent<TransformComponent>();
        selection.Select(&entity);
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<AddComponentEvent>([this](AddComponentEvent& e) {
        const ComponentRegistry::Entry* type = componentTypes.Find(e.GetComponentTypeID());
        if (type && scene.Contains(e.GetEntity())) type->addTo(*e.GetEntity());
        return true;
    }));

    subscriptions.push_back(eventBus.Subscribe<RemoveComponentEvent>([this](RemoveComponentEvent& e) {
        if (scene.Contains(e.GetEntity())) e.GetEntity()->RemoveComponent(e.GetComponentTypeID());
        return true;
    }));

    // Uploads wait for the queue, so a big model stalls the frame.
    subscriptions.push_back(eventBus.Subscribe<ImportModelEvent>([this](ImportModelEvent& e) {
        if (Entity* model = ImportModel(e.GetPath(), scene, resources)) selection.Select(model);
        return true;
    }));

    // Everything else that points at the entity or a descendant lets go
    // first.
    subscriptions.push_back(eventBus.Subscribe<DestroyEntityEvent>([this](DestroyEntityEvent& e) {
        Entity* entity = e.GetEntity();
        if (!scene.Contains(entity)) return true;

        if (selection.GetEntity() && selection.GetEntity()->IsInSubtree(entity)) selection.Clear();
        for (const Entity* doomed : Scene::GetSubtree(*entity)) renderer.ForgetEntity(doomed);
        scene.DestroyEntity(entity);
        return true;
    }));
}

// In the order the Add Component menus list them.
void App::RegisterComponents() {
    componentTypes.Register<TransformComponent>({ .icon = ICON_FA_UP_DOWN_LEFT_RIGHT });

    // A sphere with a material of its own, so it can be edited, and a
    // bounding volume, so it is culled.
    componentTypes.Register<MeshComponent>({
        .icon = ICON_FA_DRAW_POLYGON,
        .addTo = [this](Entity& entity) {
            Material* material = scene.CreateMaterial({ .metallicFactor = 0.0f, .roughnessFactor = 0.5f });
            entity.AddComponent<MeshComponent>(LoadPrimitive(resources, SpherePrimitive {}), material);
            entity.AddComponent<BoundingVolumeComponent>();
        }
    });

    componentTypes.Register<BoundingVolumeComponent>({ .icon = ICON_FA_VECTOR_SQUARE });

    componentTypes.Register<LightComponent>({ .icon = ICON_FA_LIGHTBULB });

    componentTypes.Register<CameraComponent>({ .icon = ICON_FA_VIDEO });

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

// Menu entries and panels appear in the order added.
void App::BuildGui() {
    ImGuiMenu& menuBar = gui.GetMenuBar();

    // ----
    // File
    // ----
    ImGuiMenu& file = menuBar.GetMenu("File");
    ImGuiMenu& newScene = file.GetMenu("New Scene");
    newScene.AddItem("3D", [this] { eventBus.Enqueue<NewSceneEvent>(SceneDimension::ThreeD); });
    newScene.AddItem("2D", [this] { eventBus.Enqueue<NewSceneEvent>(SceneDimension::TwoD); });
    // Every file in the scene directory; a save lists it again.
    openSceneMenu = &file.GetMenu("Open Scene");
    UpdateSceneMenu();
    saveScenePanel = gui.AddPanel<SaveScenePanel>("Save Scene As", eventBus, scene);
    // Ctrl+S is bound in SubscribeToEvents().
    file.AddItem("Save Scene", [this] { SaveOrAskForName(); }, { .shortcut = "Ctrl+S" });
    file.AddItem("Save Scene As...", [this] { saveScenePanel->Open(); });
    file.AddSeparator();
    ImGuiMenu& importModel = file.GetMenu("Import Model");
    const std::filesystem::path modelRoot = FileSystem::getPath("models");
    const std::vector<std::filesystem::path> models = FindModels();
    for (const std::filesystem::path& model : models) {
        importModel.AddItem(model.lexically_relative(modelRoot).generic_string(), [this, model] { eventBus.Enqueue<ImportModelEvent>(model); });
    }
    if (models.empty()) importModel.AddItem("No models found", {}, { .enabled = [] { return false; } });
    file.AddSeparator();
    // Asks to quit, as Esc does (see SubscribeToEvents()).
    file.AddItem("Quit", [this] { eventBus.Enqueue<QuitApplicationEvent>(); }, { .shortcut = "Esc" });

    // ----
    // Edit
    // ----
    // As the Delete key does (see SubscribeToEvents()).
    menuBar.GetMenu("Edit").AddItem("Delete", [this] { eventBus.Enqueue<DestroyEntityEvent>(*selection.GetEntity()); }, {
        .shortcut = "Del",
        .enabled = [this] { return selection.GetEntity() != nullptr; }
    });

    // ----
    // View
    // ----
    ImGuiMenu& view = menuBar.GetMenu("View");
    view.AddPanelItem(*gui.AddPanel<ViewportPanel>("Viewport", renderer, selection, camera, scene, eventBus));
    view.AddPanelItem(*gui.AddPanel<ScenePanel>("Scene", scene, componentTypes, selection, eventBus));
    view.AddPanelItem(*gui.AddPanel<InspectorPanel>("Inspector", selection, componentTypes, resources, eventBus));
    view.AddPanelItem(*gui.AddPanel<EnginePanel>("Engine", camera));
    view.AddSeparator();
    // Display settings, read when the frame is drawn, so set directly.
    view.AddItem("Grid", [this] { renderer.SetShowGrid(!renderer.GetShowGrid()); },
        { .checked = [this] { return renderer.GetShowGrid(); } });
    view.AddItem("Bounding Volumes", [this] { renderer.SetShowBoundingVolumes(!renderer.GetShowBoundingVolumes()); },
        { .checked = [this] { return renderer.GetShowBoundingVolumes(); } });
    view.AddSeparator();
    view.AddPanelItem(*gui.AddPanel<DemoWindowPanel>("ImGui Demo"));
}

// ------
// Scenes
// ------
bool App::OpenScene(const std::filesystem::path& path) {
    // Into a scene of its own, so a file that fails leaves this one.
    Scene loaded;
    if (!LoadScene(path, loaded, resources, componentTypes)) return false;
    ReplaceScene(std::move(loaded), path);
    return true;
}

// As Unity starts one: a camera, and in 3D a sun.
void App::NewScene(SceneDimension dimension) {
    Scene created;
    created.SetDimension(dimension);

    Entity& cameraEntity = created.CreateEntity("Main Camera");
    auto* cameraTransform = cameraEntity.AddComponent<TransformComponent>();
    if (dimension == SceneDimension::ThreeD) {
        const glm::vec3 eye { 0.0f, 1.6f, 7.5f };
        cameraTransform->SetPosition(eye);
        cameraTransform->SetRotation(FacingDirection(glm::vec3(0.0f, 0.5f, 0.0f) - eye));
        cameraEntity.AddComponent<CameraComponent>(CameraProjection::Perspective);

        Entity& sun = created.CreateEntity("Sun");
        sun.AddComponent<TransformComponent>()->SetRotation(FacingDirection({ 0.5f, -1.0f, -0.6f }));
        sun.AddComponent<LightComponent>(LightType::Directional)->SetIntensity(3.0f); // lux
    } else {
        cameraTransform->SetPosition({ 0.0f, 0.0f, 10.0f });
        cameraEntity.AddComponent<CameraComponent>(CameraProjection::Orthographic);
    }
    created.SetCamera(&cameraEntity);

    ReplaceScene(std::move(created), {});
}

bool App::SaveScene(const std::filesystem::path& path) {
    // Saved under a new name, the scene takes the file's.
    if (path != scenePath) scene.SetName(path.stem().string());
    if (!GNC::SaveScene(path, scene, componentTypes)) return false;

    scenePath = path;
    window.SetTitle("GNC Engine - " + scene.GetName());
    UpdateSceneMenu();
    return true;
}

void App::SaveOrAskForName() {
    if (scenePath.empty()) saveScenePanel->Open();
    else eventBus.Enqueue<SaveSceneEvent>(scenePath);
}

// Whatever points into the old scene lets go first, as for
// DestroyEntityEvent. The GPU holds nothing of its entities or materials.
void App::ReplaceScene(Scene&& next, std::filesystem::path path) {
    selection.Clear();
    for (const Entity* entity : scene.GetEntities()) renderer.ForgetEntity(entity);
    scene = std::move(next);
    scenePath = std::move(path);

    window.SetTitle("GNC Engine - " + scene.GetName());
    ResetEditorCamera();
}

// Until there is a play mode, the scene's camera is only where the editor
// starts looking from. A 2D scene's editor camera is orthographic and looks
// down -Z, whatever its scene camera does, and only pans and zooms.
void App::ResetEditorCamera() {
    const bool twoD = scene.GetDimension() == SceneDimension::TwoD;
    cameraController.SetTwoD(twoD);

    if (!MoveCameraToSceneCamera()) {
        if (twoD) camera.SetPosition({ 0.0f, 0.0f, 10.0f });
        else if (camera.GetProjectionType() == ProjectionType::Orthographic) camera.SetPerspective(Camera::DefaultFov, camera.GetAspectRatio(), 0.1f, 1000.0f);
    }
    if (twoD) {
        camera.SetRotation(Camera::DefaultYaw, 0.0f);
        if (camera.GetProjectionType() != ProjectionType::Orthographic) camera.SetOrthographic(5.0f, 0.1f, 1000.0f);
    }
}

bool App::MoveCameraToSceneCamera() {
    Entity* entity = scene.GetCamera();
    const CameraComponent* sceneCamera = entity ? entity->GetComponent<CameraComponent>() : nullptr;
    if (!sceneCamera) return false;

    const TransformComponent* transform = entity->GetComponent<TransformComponent>();
    const glm::mat4 world = transform ? transform->GetWorldMatrix() : glm::mat4(1.0f);
    const glm::vec3 position(world[3]);
    camera.SetPosition(position);
    camera.LookAt(position - glm::vec3(world[2]));

    if (sceneCamera->GetProjection() == CameraProjection::Perspective) {
        camera.SetPerspective(glm::degrees(sceneCamera->GetYFov()), camera.GetAspectRatio(), sceneCamera->GetZNear(), sceneCamera->GetZFar());
    } else {
        camera.SetOrthographic(sceneCamera->GetYMag(), sceneCamera->GetZNear(), sceneCamera->GetZFar());
    }
    return true;
}

void App::UpdateSceneMenu() {
    openSceneMenu->Clear();
    const std::vector<std::filesystem::path> scenes = FindScenes();
    for (const std::filesystem::path& path : scenes) {
        openSceneMenu->AddItem(path.stem().string(), [this, path] { eventBus.Enqueue<OpenSceneEvent>(path); },
            { .checked = [this, path] { return scenePath == path; } });
    }
    if (scenes.empty()) openSceneMenu->AddItem("No scenes found", {}, { .enabled = [] { return false; } });
}

} // namespace GNC
