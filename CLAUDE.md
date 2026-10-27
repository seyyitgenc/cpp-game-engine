# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project state

A C++23 game engine being ported from OpenGL to Vulkan, following the Khronos Vulkan Tutorial's "Building a Simple Engine" series but adapted to this engine's own design. Active work happens on the `vulkan` branch; `opengl-develop` holds the old OpenGL engine. `ReadME.md` describes the Vulkan engine: features, build steps, controls and the TODO list.

## Working conventions

- **Answers:** if you are unsure about something, say so plainly instead of guessing or presenting a guess as fact.
- **Code comments:** comment the why, not the what: constraints that would break silently, library quirks, ordering, units. Do not restate the code, explain library basics, or repeat this file. Keep class header comments to a few lines.
- **Section comments:** separate logical sections of a file or function with a banner comment, the title in between two dash lines as long as the title:

  ```cpp
  // --------------------
  // Orbiting Point Light
  // --------------------
  ```

## Build and run

The toolchain is Windows + MSYS2 MinGW64 (g++ 15) + CMake (Unix Makefiles) + the Vulkan SDK (`VULKAN_SDK` must be set; `slangc` comes from it). assimp is MSYS2's package (`mingw-w64-x86_64-assimp`, 6.0). In Git Bash, put MinGW on PATH first:

```sh
export PATH="/c/msys64/mingw64/bin:$PATH"
cmake -S . -B build                      # Debug by default; -DCMAKE_BUILD_TYPE=Release for bin/release
cmake --build build -j8
cd bin/debug && ./engine.exe             # or ./run.sh
```

- Output goes to `bin/debug/` (or `bin/release/`). The `shaders` CMake target compiles every `shaders/*.slang` into `bin/<config>/shaders/<name>.vert.spv` and `<name>.frag.spv`. The `fonts` target copies the icon font to `bin/<config>/fonts/`. The `models` target copies every folder under `resources/objects` that holds a `.gltf`, `.glb`, `.obj`, `.fbx` or `.dae` (`MODEL_EXTENSIONS`) to `bin/<config>/models/` (it needs CMake 3.26 for `copy_directory_if_different`).
- **Resource paths resolve against the exe's directory**, not the working directory. `main()` calls `FileSystem::init(argv[0])` (`src/engine/Utils/FileSystem.hpp`), and resources build their paths with `FileSystem::getPath("shaders/...")`. A bare name from a PATH lookup in `argv[0]` would resolve to the working directory instead.
- **Re-run the configure step after adding a `.cpp`, `.slang` or model file.** Sources, shaders and model folders are collected with `file(GLOB)`, which only picks up new files when CMake reconfigures.
- Configuring writes the build type into `src/configuration/config.h`, which is tracked, so configuring a Release build changes it. Don't commit that change.
- Warnings are `-Wall -Wextra -Wpedantic`; keep the build warning-free.
- There are no tests and no lint step. `.clang-format` (WebKit based) exists, but existing files are not uniformly formatted with it, so do not reformat code you are not changing.
- **Debug builds enable `VK_LAYER_KHRONOS_validation`** when it is installed. Messages print to stdout as `[Vulkan] ...`. A clean run prints no validation output; treat any as a bug.

## Commits

Commit messages are in Turkish. Follow the commits on the `vulkan` branch; older history is in English and does not follow these rules.

- **Subject:** `type(Scope,Scope): summary.`
  - `type` is `feat`, `fix` or `refactor`.
  - Scopes name the engine areas the commit touches, after their `src/engine/` directories (`Renderer`, `Platform`, `Event`, `UI`, ...), comma separated without spaces.
  - The summary says what was done in the past tense passive (`eklendi`, `getirildi`, `kaldirildi`, `entegre edildi`) and ends with a period.
- **No Turkish letters:** write `c s g i o u` for `ç ş ğ ı ö ü` (`calisir hale getirildi`, not `çalışır hale getirildi`). English technical terms stay as they are and take Turkish suffixes after an apostrophe: `pass'ler`, `semaphore'lari`, `backend'lerle`.
- **Body:** a bulleted list (`- `), one bullet per change, usually starting with the class or file it is about (`- Swapchain: ...`). Wrap lines at about 76 columns and indent continuation lines by two spaces.
- **Scope of a commit:** one finished piece of work. If the change makes this file wrong, update `CLAUDE.md` in the same commit.
- Commits Claude makes end with its `Co-Authored-By` trailer.
- Commit only when asked, and push only when asked.

Example:

```
feat(Renderer): Frames in flight, kamera UBO ve FrameContext eklendi.

- FrameData.hpp: frame basina command buffer, fence, semaphore, kamera
  uniform buffer (kalici map'li) ve descriptor set. MaxFramesInFlight = 2.
- VulkanHelpers: createBuffer eklendi.
```

## Architecture

### Ownership and the frame loop

`App` (`src/engine/App.hpp`) owns everything as plain members. Declaration order is load-bearing: C++ destroys in reverse, which gives the teardown Vulkan needs. The order is EventBus → Window → VulkanContext → upload command pool → TextureTable → ResourceManager → Camera → CameraController → Renderer → Scene → ComponentRegistry → Selection → ImGuiLayer → event subscriptions. When adding a member, place it after everything it depends on.

`App::Run()` computes the frame time (capped at `App::MaxDeltaTime`), then runs three traversals in order, as OSG's viewer does:

1. **`EventTraversal(dt)`:** `window.PollEvents()`; unless minimized, enqueue a `FrameEvent(dt)` behind the frame's input; then `eventBus.Flush()`. Every event listener runs here and nowhere else.
   - While minimized (framebuffer 0x0, `Window::IsMinimized`), `Run` then waits for events and skips the other two traversals.
2. **`UpdateTraversal(dt)`:** `cameraController.Update(dt)` and `scene.Update(dt)` (which calls each component's `Update`).
3. **`RenderTraversal()`:**
   - `renderer.Resize(extent)` if a `WindowResizeEvent` arrived or the last `Render()` returned false (swapchain out of date). `Resize` rebuilds the swapchain *and every render pass*.
   - `gui.Render()` builds the UI, then `renderer.Render(scene.GetEntities())` draws the frame, then `gui.RenderPlatformWindows()`.
   - The Viewport panel calls `renderer.SetSceneExtent()` from inside `gui.Render()`. A size change waits for the GPU and rebuilds every render pass, as `Resize` does.

### Events and input (`src/engine/Event/`, `Platform/Window`)

- **Publishing:** GLFW callbacks in `Window.cpp` only enqueue events onto the `EventBus`. Resize uses `EnqueueCoalesced`, so a drag yields one event.
- **Subscribing:** listeners use `bus.Subscribe<E>(lambda, priority)` and must keep the returned `EventSubscription`; dropping it unsubscribes. Owners store tokens in a `std::vector<EventSubscription>` member.
- **Handled events:** returning true from a listener stops delivery to lower-priority listeners. This is how `ImGuiLayer` swallows input before the camera sees it.
- **Event kinds:** each kind is its own class, following OSG's GUIEventAdapter set. The mapping is PUSH/RELEASE → `MouseButtonPressed/ReleasedEvent`, DOUBLECLICK → `MouseDoubleClickEvent`, MOVE → `MouseMovedEvent`, DRAG → `MouseDraggedEvent`, KEYDOWN/KEYUP → `KeyPressed/ReleasedEvent`, SCROLL → `MouseScrolledEvent`, RESIZE → `WindowResizeEvent`, CLOSE_WINDOW → `WindowCloseEvent`, FRAME → `FrameEvent` and QUIT_APPLICATION → `QuitApplicationEvent`. Game-defined events use `GNC_EVENT_TYPE` with `EventCategory::User`.
  - Unlike OSG, `Window` sends DOUBLECLICK *after* the second press, and DRAG *alongside* MOVE, never in place of them.
  - Beyond OSG's set: `WindowFocusEvent`, `CharTypedEvent`, and `MouseEnterEvent`/`MouseLeaveEvent` (the cursor crossing the window edge, which the ImGui backend needs).
- **Scene requests** (`SceneEvents.hpp`): the UI creates and destroys entities and adds or removes components only by enqueuing `CreateEntityEvent` (optionally under a parent), `DestroyEntityEvent` (which takes the descendants too), `AddComponentEvent` or `RemoveComponentEvent`, which App applies at the next `Flush()`. It is mid-loop over the entities when a menu item is picked. A property (a value, a name, the active flag) it may set directly.
  - Requests made in one frame can meet in one `Flush()`, a destroy ahead of another request for the same entity. So every listener checks `Scene::Contains()` before using an event's `Entity*`.
- **Held keys come from events:** track `KeyPressed`/`KeyReleased` (ignoring repeats) and clear on focus loss, as `CameraController` does. `Window::IsKeyDown` bypasses the bus, so ImGui could not swallow the key. Never return true for a release event. Key codes are GLFW's, untranslated.
- **Camera vs editor keys:** the camera moves only while the right button is held. Its listeners run at `CameraController::EventPriority` (100), and while looking it swallows its movement keys, so the Viewport's W/E/R tool shortcuts below it do not fire mid-flight.

### Vulkan setup

- **`Renderer/VulkanContext`** creates the instance, surface, GPU choice and device. It requires Vulkan 1.3 with `dynamicRendering`, `synchronization2`, `samplerAnisotropy`, `shaderSampledImageArrayDynamicIndexing` (for `TextureTable`) and `fillModeNonSolid` (for DebugDrawPass's wireframe). One queue family does graphics, present and uploads.
- **Vulkan-Hpp style:** all objects are `vk::raii`, and `VULKAN_HPP_NO_STRUCT_CONSTRUCTORS` is defined, so structs use designated initializers.
  - GCC warns when a designated initializer skips a field with no default value, such as the deprecated `DeviceCreateInfo` layer fields; use the setters for those structs.
  - `vk::OutOfDateKHRError` is caught in `Swapchain` and turned into "recreate".
- **`Renderer/Renderer`** owns the swapchain, the per-frame data and the render passes:
  - `MaxFramesInFlight = 2`. Each `FrameData` holds a command buffer, a fence, an image-available semaphore, mapped camera and light uniform buffers, its descriptor set 0, and a pick readback buffer (see Picking).
  - Render-finished semaphores live in `Swapchain`, one per swapchain image, not per frame.
- **Load-time uploads** go through `uploadBuffer()` in `Utils/VulkanHelpers`: staging buffer, copy, then wait for the queue.

### Deferred pipeline

`RenderPassManager` orders passes by declared dependencies. **Passes must call `AddDependency`**; without it, order follows `unordered_map` iteration and is effectively random.

1. **GeometryPass** draws every visible `MeshComponent` into the G-buffer: base color, octahedral-encoded normal, occlusion/roughness/metallic, emissive, an `R32_UINT` entity id (for picking, not lighting), and D32 depth. The formats live in the `GBuffer` struct (`GeometryPass.hpp`) and must match `shaders/common/gbuffer.slang`.
   - Materials: each frame it writes the materials it draws with into that frame's `GpuMaterial` storage buffer (set 2, index 0 = defaults, at most `MaxMaterials`). Their textures are `TextureTable` slots (set 1).
   - Two pipelines from one shader: opaque, and alpha-masked (`ALPHA_MASK` specialization constant, the only one that discards), drawn after the opaque draws. The cull mode is dynamic state, `eNone` for double-sided materials, whose back faces the shader lights from their own side.
2. **LightingPass** runs a fullscreen triangle. It rebuilds world position from depth with `inverseViewProjection`, loops over every light with the Cook-Torrance BRDF (`shaders/common/brdf.slang`), adds flat ambient and emissive, and writes linear HDR (`R16G16B16A16_SFLOAT`).
3. **PostProcessPass** applies exposure and the Khronos PBR Neutral tonemapper into its own output, the *scene image* (`R8G8B8A8_UNORM`, `Renderer::GetSceneView()`). The shader applies the sRGB encoding itself, then draws the editor's outlines from the entity ids and depth (see Outlines). `EndPass` leaves the image in `eShaderReadOnlyOptimal` for ImGui, which shows it in the Viewport panel.
   - UNORM on purpose: ImGui copies texels to the swapchain as they are. An sRGB scene image would be decoded when sampled and show too dark.
   - The swapchain *prefers* a UNORM format too. ImGui's colors are already sRGB encoded; an sRGB swapchain would encode them again and wash the UI (and the scene in it) out.
4. **DebugDrawPass** draws `Renderer::GetDebugDraw()`'s bounding volumes into the scene image as wireframe: triangle strips (primitive restart between strips) with polygon mode line. It depth tests against the G-buffer depth, attached read-only, drawing every shape twice: `eGreater` at `HiddenOpacity`, then `eLessOrEqual` at full strength. It is skipped when there is nothing to draw, and otherwise returns the scene image to `eShaderReadOnlyOptimal`.
   - Boxes and spheres share a unit mesh each (`BoxStrip`, a latitude-band sphere), placed by the push constant model matrix. A capsule is three draws: a unit cylinder along its segment and a unit hemisphere at each end, the one at the start mirrored. A hull's triangles become a mesh of their own the first time it is drawn, kept by `BoundingVolumeComponent::GetFitId()` and dropped `MaxFramesInFlight` frames after it was last drawn.
   - Colors are sRGB encoded, as the scene image holds them. The shader pulls every point 0.1 % of its distance towards the camera, so a wire lying on a surface does not fight it for depth.
   - The Renderer adds the visible entities' volumes while `SetShowBoundingVolumes(true)` (the Viewport toolbar button, View > Bounding Volumes), the selection's in its outline color, and clears the list after recording each frame.
5. **ImGuiPass** is the only pass that writes the swapchain image (`FrameContext::targetImage`/`targetView`). It does the acquire barrier, clears the image, and draws ImGui's draw data with `ImGui_ImplVulkan_RenderDrawData`. It owns no GPU objects (see ImGui below).

How passes work together:
- **Sizes:** the G-buffer, the HDR target and the scene image are all `Renderer`'s *scene extent* (the Viewport panel's size, `SetSceneExtent()`), not the swapchain's. Only ImGuiPass is swapchain-sized. The lighting shader takes the size from `gDepth.GetDimensions`, so the targets must be exactly the scene's size.
- **Layout transitions:** each pass does its own `pipelineBarrier2` in `BeginPass`/`EndPass`. Offscreen targets are shared by all frames in flight; the barrier at the start of each pass waits for the previous frame's readers, which for the scene image are ImGui's (also in platform windows).
  - The swapchain image stays in `eColorAttachmentOptimal` through every pass. `Renderer::Render` transitions it to `ePresentSrcKHR` after the last one, so a pass can be added at the end without moving that barrier.
  - GeometryPass's depth barrier waits on the fragment shader (the lighting and post-process passes sample depth) and on the depth tests (DebugDrawPass).
- **Descriptor sets:** set 0 is always the global set (binding 0 camera, binding 1 lights) and every pipeline layout includes it. Set 1 holds the pass's inputs, which the fullscreen passes read with `Texture2D.Load()`: sampled images, no samplers. GeometryPass's set 1 is the `TextureTable` instead (combined image samplers).
- **Per-draw data** goes in push constants (`GeometryPushConstants`: model matrix, material index and entity id, which must stay ≤128 bytes); material data goes in the material buffer.
- **Textures** (`Renderer/TextureTable`): one array of `Capacity` combined image samplers, which materials index. Free slots hold a white texture, so the whole array is always valid without descriptor indexing features; slot 0 is white and slot 1 a flat normal, the defaults for a missing map.
  - `Add`/`Remove` write the set, which is only legal while no submitted frame uses it. A `Texture` adds itself in `doLoad()` right after `uploadImage()` waited for the queue; keep any other caller to that rule.
- **Picking:** the entity id is 1 + the draw's index in that frame's `FrustumCulling` visible list, as ordered for the outlines (0 = nothing drawn), so ids mean nothing outside their frame. GeometryPass leaves the id target in `eShaderReadOnlyOptimal` for the post-process pass.
  - After all passes, `Renderer::RecordPick` transitions it to `eTransferSrcOptimal` and copies up to two texels into the frame's readback buffer, along with a copy of that frame's visible list. One texel is for a click (`RequestPick`), one for the cursor (`SetHoverPixel`, every frame it is over the scene). GeometryPass's first barrier therefore waits on both the fragment shader and the copy.
  - After that frame's fence, the click answer comes from `TakePickResult()`, once, and the hover answer from `GetHoveredEntity()` (nullptr for the background). The Viewport sees them `MaxFramesInFlight + 1` frames after the request.
- **Outlines:** `Renderer::SetOutlinedEntities(selected, hovered)` names entities, each standing for its whole subtree. After culling, `Renderer::OrderForOutlines()` reorders the visible list (selected only, both, hovered only), so each subtree's ids form one range, which PostProcessPass gets as `[begin, end)`. Its shader marks pixels within `OUTLINE_RADIUS` outside each silhouette, after tonemapping and encoding, and skips the outline work when both ranges are empty.
  - Only pixels behind the outlined entity (by linear depth, within `OUTLINE_DEPTH_TOLERANCE`) take its outline. Otherwise everything in front of a selected building would look selected.
- **Shader modules** are loaded in `Renderer::SetupRenderPasses()` by constructing `Shader` objects directly, not through `ResourceManager`. The manager caches by id only, so the vertex and fragment stages of the same shader would collide in its cache.

### ImGui (`src/engine/UI/ImGuiLayer`)

- **Library:** Dear ImGui's docking branch, vendored as plain files in `src/external/ImGui` (core plus `backends/imgui_impl_glfw` and `imgui_impl_vulkan`, unmodified), built as the `imgui` static library. `CMakeLists.txt` records the upstream commit and how to update.
- **ImGuizmo** (`src/external/ImGuizmo`, the `imguizmo` library) draws the Viewport's gizmo. It includes `imgui_internal.h`, so update it with ImGui; `CMakeLists.txt` records the commit. `ImGuiLayer::NewFrame()` calls `ImGuizmo::BeginFrame()`, which advances `ImGuizmo::IsOver()` to the frame that just ended.
- **Icons:** `ImGuiLayer::LoadFonts()` merges Font Awesome 6 Free Solid (`src/external/FontAwesome`) into the default font, and `UI/Icons.hpp` names its glyphs (`ICON_FA_*`, from `src/external/IconFontCppHeaders`). The header and the font must come from the same Font Awesome release; `CMakeLists.txt` records both.
  - The default font keeps its explicit 13 px size: ImGui asserts when a font with explicit metrics merges into one whose size was left implicit.
- **Ownership:** `ImGuiLayer` owns the context and both backends. It is created after the `Renderer` (its pipeline is built for the swapchain format). On destruction it waits for the GPU and destroys its panels, so they can still free their textures, before shutting the backends down.
- **Panels:** the UI is made of `ImGuiPanel`s (`UI/ImGuiPanel.hpp`), each with a name, a visibility flag and a `Render()` that opens its own window with `ImGui::Begin(GetName().c_str(), GetVisibleFlag())`. Passing the flag gives the window a close button. `ImGuiLayer` is their manager:
  - `gui.AddPanel<T>(name, args...)` constructs `T(name, args...)`, like `RenderPassManager::AddRenderPass`. App adds its panels and menus in `App::BuildGui()`.
  - `gui.Render()` runs the whole ImGui frame: NewFrame, the main menu bar, a dockspace over the rest of the main window (not passthru), every *visible* panel in the order added, then `ImGui::Render()`. ImGui calls are only valid inside a panel's `Render()`.
  - A panel whose `DocksInCentralNode()` returns true docks into the dockspace's central node the first time its window exists (`SetNextWindowDockID`, `ImGuiCond_FirstUseEver`). After that, `imgui.ini` decides.
  - App-specific panels live in `UI/Panels/` (`ViewportPanel`: the scene image, click to select, gizmo; `ScenePanel`: the entity hierarchy as a tree with icons, each entity opening onto its components and then its children, and opening up to a selection made elsewhere; `InspectorPanel`: the selected entity's properties; `EnginePanel`: FPS and camera position; `DemoWindowPanel`: ImGui's demo window, hidden at start).
  - Panels hold references to what they show, so whatever a panel shows must be an `App` member declared before `gui`.
- **Scene, Viewport and Inspector panels** share App's `Selection` (one `Entity*`). The Inspector draws every component from its properties with `UI/PropertyEditor` (see Properties below).
- **Viewport:** `ViewportPanel` shows the scene image and sizes the scene to its content area with `renderer.SetSceneExtent()`.
  - It registers the image with `ImGui_ImplVulkan_AddTexture` (the backend brings its own sampler). It re-registers whenever `Renderer::GetSceneVersion()` changes, since every pass rebuild replaces the image after a wait for the GPU. It compares versions, not view handles, because a recreated view can reuse the old handle value.
  - The backend's descriptor pool has room for `ImGuiLayer::MaxUserTextures` such textures on top of the font atlas.
  - An `InvisibleButton` over the image takes left clicks, so a floating viewport is not dragged. A click that did not pass ImGui's drag threshold picks at the press position (see Picking); the panel applies the answer to `Selection` when it arrives, and the background clears it. While the image is hovered, the panel sends the hover pixel and calls `SetNextFrameWantCaptureMouse(false)`, so the layer lets mouse events through to the camera. Holding the left button on it makes ImGui capture the keyboard (an active item); right-drag + WASD is unaffected.
  - **Click target** (`ViewportPanel::ClickTarget`): a click on a model selects its root first. Once anything in that tree is selected, a click selects the entity drawn there.
  - Every frame it sets the outlines to the selection and to what a click on the hovered entity would select.
  - **Gizmo:** the selection, if it has a transform, gets an ImGuizmo gizmo. The tool is chosen with W/E/R (a bus listener below `CameraController::EventPriority`) or the toolbar over the image, which the button lets overlap it (`SetNextItemAllowOverlap`). The gizmo works on the world matrix; the result is brought back into the parent's space, and only what the tool changes is written back to `TransformComponent`.
    - ImGuizmo only takes a press while no ImGui item is hovered or active. So while `ImGuizmo::IsOver()` (last frame's), the panel leaves out the button and sets `ImGuiWindowFlags_NoMove`. A press in the first frame or two after reaching the gizmo can still miss it.
    - ImGuizmo ends a drag only inside `Manipulate()`. When the selection has no transform (deleted mid-drag), the panel ends the drag with `Enable(false)`/`Enable(true)`, or `IsOver()` would stay true.
    - ImGuizmo expects OpenGL's clip space, Y up: the panel undoes `Camera`'s Y flip on a copy of the projection. The 0..1 depth needs no change.
  - The toolbar's last button shows and hides the bounding volumes (see DebugDrawPass).
  - Dragged out into its own OS window, the viewport still shows the scene, picks and manipulates (ImGui gets that window's input), but that input bypasses the bus, so the camera does not respond there.
- **Menu bar:** `gui.GetMenuBar()` is an `ImGuiMenu` tree (`UI/ImGuiMenu.hpp`): `GetMenu(label)` finds or adds a submenu, `AddItem(label, action, { .shortcut, .enabled, .checked })` adds a command, `AddPanelItem(panel)` a check mark item that shows and hides a panel, and `AddSeparator()` a line.
  - The bar's submenus are its dropdown menus; an item added to the bar itself is a button on it.
  - The shortcut text is display only; bind the key with an event listener.
  - Commands run inside the ImGui frame, before the frame is drawn. Anything outside the UI, other than a property or a display setting (View > Bounding Volumes), should be changed by enqueuing an event (File > Quit enqueues `QuitApplicationEvent`), not directly.
  - Which panels are visible is not saved; `imgui.ini` only keeps window positions and docking.
- **Input comes from the bus**, not from GLFW callbacks: the GLFW backend is initialized with `install_callbacks = false`. Listeners at `ImGuiLayer::EventPriority` pass each event to the matching `ImGui_ImplGlfw_*Callback` and return `io.WantCaptureMouse`/`WantCaptureKeyboard` to swallow it. Releases, focus and enter/leave are never swallowed.
  - While the camera has the cursor captured, the layer ignores mouse input and sets `ImGuiConfigFlags_NoMouse`.
  - Platform windows the backend creates (panels dragged out of the main window) get the backend's own callbacks; their input never reaches the bus.
- **Multi-viewport** is on (`ImGuiLayer::EnableViewports`). Those windows have their own swapchains, which the backend renders and presents in `RenderPlatformWindows()` on the same queue.
- **Pipeline:** the backend's main-window pipeline is rebuilt only when the swapchain format changes, not on every resize. Rebuilding it after a platform window exists leaves the backend's viewport pipeline info pointing at freed memory (`imgui_impl_vulkan`, 1.93.1 WIP).
- **`imgui.ini`** is saved next to the executable (`FileSystem::getPath`).

### C++ ↔ shader contract

- **Shared structs** are mirrored in `Renderer/ShaderTypes.hpp` (`GpuLight`, `LightsUniform`, `GpuMaterial`, push constants), plus `CameraUniform` in `Camera/Camera.hpp`. Each mirror has `static_assert`s on offsets and size. Change both sides together.
  - Layout rule: every `vec3` is followed by a 4-byte scalar, so std140 and C++ agree without hidden padding. In std430 storage buffers (`GpuMaterial`), pad with scalars, not a `uint3`, which aligns to 16.
- **Vertex** (`Resource/Types/Mesh.hpp`) is position, normal, UV and a glTF tangent (xyz, handedness in w) at locations 0–3. The bitangent is `cross(normal, tangent.xyz) * w` and runs along −v, the image's up. That is what Blender's exporter writes, and what `GenerateTangents()` matches.
  - To check the shader side, compile with `slangc ... -reflection-json out.json` and compare the offsets.
- **Slang conventions:** each `shaders/*.slang` file has `vertMain`/`fragMain`. `shaders/common/*.slang` are modules (`import common.frame;`, built with `-I shaders`).
  - The SPIR-V entry point name is `main`.
  - Use `SV_VulkanVertexID`. Plain `SV_VertexID` pulls in `gl_BaseVertex`, which needs a device feature that is not enabled.
- **Matrices:** upload glm matrices unchanged and multiply with `mul(M, v)`. In shaders, `m[row][col]` indexes the mathematical matrix.
- **Coordinate conventions:**
  - `Camera`'s projection already flips Y and uses 0..1 depth, so never flip again in a UBO or viewport.
  - Front faces are counter-clockwise. Depth clears to 1 with `LESS`.
  - Lights shine down their entity's local -Z axis.

### ECS, scene and resources

- **`Entity`** owns its components. A component counts as active only after `Initialize()`; the renderer skips light components that are not active.
  - Every concrete component declares `GNC_COMPONENT_TYPE(Class, "Name")`, as events declare `GNC_EVENT_TYPE`.
  - `Scene::CreateEntity` initializes the entity immediately, so components added afterwards are initialized as they are added.
  - `Scene` owns materials behind `unique_ptr`, because `MeshComponent` holds raw `Material*` pointers.
  - **Hierarchy:** an entity has a parent and children, set only by `Scene` (`CreateEntity(name, parent)`). The scene's entity list stays flat, in creation order. There is no reparenting yet.
    - `Scene::MakeUniqueName` makes names unique among siblings, not scene-wide.
    - `IsActiveInHierarchy()` is false under an inactive ancestor; update, culling and lights use it.
  - **Transforms:** `TransformComponent` values are local to the nearest ancestor that has a transform. `GetWorldMatrix()` walks up through the ancestors on every call (nothing is cached), and everything that places things in the world uses it: culling, GeometryPass, lights and the gizmo.
  - **Destroying:** `Scene::DestroyEntity()` takes the whole subtree. App's `DestroyEntityEvent` listener first clears `Selection` if it is in the subtree, and calls `Renderer::ForgetEntity()` (pending picks, the untaken result, the hovered and outlined entities) for each entity in it. Anything new that keeps an `Entity*` must be cleared there too. The GPU holds nothing of an entity's, so no wait is needed. Its materials stay in the scene.
- **`ComponentRegistry`:** a new component type needs a `Register<T>` line in `App::RegisterComponents()` to appear in the Add Component menus and the Inspector.
- **Properties** (`ECS/Component/Property.hpp`): the editor reaches a component's values only through its `static void Reflect(PropertyList<T>&)`; there are no hand-written editors.
  - Describe what the component stores (a rotation is the `glm::quat`, not Euler angles), so scene files can later save it exactly.
  - Mesh lists its material's factors, maps, alpha mode and double-sidedness as its own properties, so editing a shared material changes every mesh using it.
  - A new kind of value needs a `PropertyValue` alternative and a widget in `UI/PropertyEditor`'s `EditValue`.
- **Lights:** `LightComponent` uses glTF `KHR_lights_punctual` semantics and physical units: candela for point and spot lights, lux for directional. Exposure is 1.
  - The renderer collects lights from *all* entities each frame (lights are not culled), up to `MaxLights = 64`.
- **Culling:** `FrustumCulling` tests each mesh entity's `BoundingVolumeComponent`: a box, a sphere, a capsule or a convex hull (`Culling/BoundingVolume`, one `std::variant`), fitted in model space to the mesh's vertices and refitted when the mesh or the shape changes. It moves the frustum planes into model space, so the test is exact under any affine world matrix.
  - An entity with a mesh but no bounding volume is never culled. The Mesh entry of the Add Component menu, the model importer and `BuildScene()` add one with every mesh.
  - The capsule runs along the vertices' principal axis (power iteration on their covariance), through the center of Ritter's circle around them seen down that axis; its ends reach only as far as the round caps need. It is loose for flat or boxy meshes, up to 1.6 times the sphere's volume.
  - Code that handles every shape visits the variant with `Overloaded` (`Utils/Overloaded.hpp`), so a new shape fails to compile until each place handles it.
  - The hull is Quickhull without face merging: a point it leaves outside a face becomes a corner too, and flat meshes get their box's corners. It keeps its triangles for drawing. Fitting one to a big mesh takes up to about 0.1 s.
- **Resources:** `ResourceManager` caches resources by (type, id). A `Resource` implements `doLoad`/`doUnload`.
  - A `Mesh` is always built from `MeshData` passed as the extra `Load` argument, e.g. `resources.Load<Mesh>("sphere", CreateSphere(...))` (see `Resource/Primitives.hpp`), or by the model importer. Uploads need the transfer queue and pool in `ResourceContext`.
  - A `Texture` is an encoded image (a file, or bytes embedded in a model file) plus `TextureSettings` (color space, the glTF sampler). It decodes with stb_image to RGBA8, uploads with a blitted mip chain, and adds itself to the `TextureTable` in `ResourceContext`.
  - The manager's reload factories keep copies of the `Load` arguments, so a mesh's `MeshData` and an embedded image's bytes stay in memory.
  - `HotReloadResourceManager` exists but is not used, and pipelines are not rebuilt when a shader reloads.
- **Model import** (`Scene/ModelImporter`, assimp): File > Import Model lists the files under `bin/<config>/models` that assimp reads (`IsModelFile`) and enqueues an `ImportModelEvent`. App imports at the next `Flush()` and selects the model's root.
  - A model is one root entity, named after the file, holding an entity per assimp node at the node's local transform. assimp's root node is the file's lone root node, or one it adds above several.
  - A node's meshes and light go on the node's entity. A node with several meshes (assimp makes one per glTF primitive or OBJ material) gets a child entity per mesh, named after its material.
  - **Materials:** glTF's metallic-roughness values arrive unchanged; the occlusion map comes as `aiTextureType_LIGHTMAP`. Phong materials (OBJ, Collada, most FBX) become dielectrics: the diffuse color counts only without a diffuse texture, shininess maps to roughness, and OBJ's `map_Bump` is read as the normal map.
  - **UVs:** `aiProcess_FlipUVs` restores glTF's top-left origin, which standard OBJ files need too. LearnOpenGL's `backpack.obj` is already top-left, so its textures come out scrambled.
  - **Lights:** assimp multiplies glTF's intensity into the color; the importer takes the brightest channel back out as the intensity. glTF's range is in the node's `PBR_LightRange` metadata.
  - Missing normals are made flat and missing tangents with `GenerateTangents()`, as glTF asks; `aiProcess_CalcTangentSpace` would give bitangents along +v.
  - Mesh and texture ids are built from the file's path under `models/`, so a second import reuses them; materials are created fresh in the scene each time.
  - Unsupported things (skins, animations, cameras, morph targets, `KHR_texture_transform`, a second UV set, vertex colors, opacity maps, separate metalness and roughness maps, alpha blending, which draws opaque) are logged once per import.
  - Uploads wait for the queue, so importing stalls (Sponza: about 2.4 s).
