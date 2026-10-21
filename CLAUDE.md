# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project state

A C++23 game engine being ported from OpenGL to Vulkan, following the Khronos Vulkan Tutorial's "Building a Simple Engine" series but adapted to this engine's own design. Active work happens on the `vulkan` branch; `opengl-develop` holds the old OpenGL engine. `ReadME.md` still describes the OpenGL engine and is out of date.

## Working conventions

- **Answers:** if you are unsure about something, say so plainly instead of guessing or presenting a guess as fact.
- **Section comments:** separate logical sections of a file or function with a banner comment, the title in between two dash lines as long as the title:

  ```cpp
  // --------------------
  // Orbiting Point Light
  // --------------------
  ```

## Build and run

The toolchain is Windows + MSYS2 MinGW64 (g++ 15) + CMake (Unix Makefiles) + the Vulkan SDK (`VULKAN_SDK` must be set; `slangc` comes from it). In Git Bash, put MinGW on PATH first:

```sh
export PATH="/c/msys64/mingw64/bin:$PATH"
cmake -S . -B build                      # Debug by default; -DCMAKE_BUILD_TYPE=Release for bin/release
cmake --build build -j8
cd bin/debug && ./engine.exe             # or ./run.sh
```

- Output goes to `bin/debug/` (or `bin/release/`). The `shaders` CMake target compiles every `shaders/*.slang` into `bin/<config>/shaders/<name>.vert.spv` and `<name>.frag.spv`.
- **Resource paths resolve against the exe's directory**, not the working directory. `main()` calls `FileSystem::init(argv[0])` (`src/engine/Utils/FileSystem.hpp`), and resources build their paths with `FileSystem::getPath("shaders/...")`. A bare name from a PATH lookup in `argv[0]` would resolve to the working directory instead.
- **Re-run the configure step after adding a `.cpp` or `.slang` file.** Sources and shaders are collected with `file(GLOB)`, which only picks up new files when CMake reconfigures.
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

`App` (`src/engine/App.hpp`) owns everything as plain members. Declaration order is load-bearing: C++ destroys in reverse, which gives the teardown Vulkan needs. The order is EventBus → Window → VulkanContext → upload command pool → ResourceManager → Camera → CameraController → Renderer → ImGuiLayer → Scene → event subscriptions. When adding a member, place it after everything it depends on.

`App::Run()` computes the frame time (capped at `App::MaxDeltaTime`), then runs three traversals in order, as OSG's viewer does:

1. **`EventTraversal(dt)`:** `window.PollEvents()`; unless minimized, enqueue a `FrameEvent(dt)` behind the frame's input; then `eventBus.Flush()`. Every event listener runs here and nowhere else.
   - While minimized (framebuffer 0x0, `Window::IsMinimized`), `Run` then waits for events and skips the other two traversals.
2. **`UpdateTraversal(dt)`:** `cameraController.Update(dt)` and `scene.Update(dt)` (which calls each component's `Update`).
3. **`RenderTraversal()`:**
   - `renderer.Resize(extent)` if a `WindowResizeEvent` arrived or the last `Render()` returned false (swapchain out of date). `Resize` rebuilds the swapchain *and every render pass*.
   - `gui.Render()` builds the UI, then `renderer.Render(scene.GetEntities())` draws the frame, then `gui.RenderPlatformWindows()`.

### Events and input (`src/engine/Event/`, `Platform/Window`)

- **Publishing:** GLFW callbacks in `Window.cpp` only enqueue events onto the `EventBus`. Resize uses `EnqueueCoalesced`, so a drag yields one event.
- **Subscribing:** listeners use `bus.Subscribe<E>(lambda, priority)` and must keep the returned `EventSubscription`; dropping it unsubscribes. Owners store tokens in a `std::vector<EventSubscription>` member.
- **Handled events:** returning true from a listener stops delivery to lower-priority listeners. This is how `ImGuiLayer` swallows input before the camera sees it.
- **Event kinds:** each kind is its own class, following OSG's GUIEventAdapter set. The mapping is PUSH/RELEASE → `MouseButtonPressed/ReleasedEvent`, DOUBLECLICK → `MouseDoubleClickEvent`, MOVE → `MouseMovedEvent`, DRAG → `MouseDraggedEvent`, KEYDOWN/KEYUP → `KeyPressed/ReleasedEvent`, SCROLL → `MouseScrolledEvent`, RESIZE → `WindowResizeEvent`, CLOSE_WINDOW → `WindowCloseEvent`, FRAME → `FrameEvent` and QUIT_APPLICATION → `QuitApplicationEvent`. Game-defined events use `GNC_EVENT_TYPE` with `EventCategory::User`.
  - Unlike OSG, `Window` sends DOUBLECLICK *after* the second press, and DRAG *alongside* MOVE, never in place of them.
  - Beyond OSG's set: `WindowFocusEvent`, `CharTypedEvent`, and `MouseEnterEvent`/`MouseLeaveEvent` (the cursor crossing the window edge, which the ImGui backend needs).
- **Held keys come from events:** track `KeyPressed`/`KeyReleased` (ignoring repeats) and clear on focus loss, as `CameraController` does. `Window::IsKeyDown` bypasses the bus, so ImGui could not swallow the key. Never return true for a release event. Key codes are GLFW's, untranslated.

### Vulkan setup

- **`Renderer/VulkanContext`** creates the instance, surface, GPU choice and device. It requires Vulkan 1.3 with `dynamicRendering` and `synchronization2`. One queue family does graphics, present and uploads.
- **Vulkan-Hpp style:** all objects are `vk::raii`, and `VULKAN_HPP_NO_STRUCT_CONSTRUCTORS` is defined, so structs use designated initializers.
  - GCC warns when a designated initializer skips a field with no default value, such as the deprecated `DeviceCreateInfo` layer fields; use the setters for those structs.
  - `vk::OutOfDateKHRError` is caught in `Swapchain` and turned into "recreate".
- **`Renderer/Renderer`** owns the swapchain, the per-frame data and the render passes:
  - `MaxFramesInFlight = 2`. Each `FrameData` holds a command buffer, a fence, an image-available semaphore, mapped camera and light uniform buffers, and its descriptor set 0.
  - Render-finished semaphores live in `Swapchain`, one per swapchain image, not per frame.
- **Load-time uploads** go through `uploadBuffer()` in `Utils/VulkanHelpers`: staging buffer, copy, then wait for the queue.

### Deferred pipeline

`RenderPassManager` orders passes by declared dependencies. **Passes must call `AddDependency`**; without it, order follows `unordered_map` iteration and is effectively random.

1. **GeometryPass** draws every visible `MeshComponent` into the G-buffer: base color, octahedral-encoded normal, occlusion/roughness/metallic, emissive, and D32 depth. The formats live in the `GBuffer` struct (`GeometryPass.hpp`) and must match `shaders/common/gbuffer.slang`.
2. **LightingPass** runs a fullscreen triangle. It rebuilds world position from depth with `inverseViewProjection`, loops over every light with the Cook-Torrance BRDF (`shaders/common/brdf.slang`), adds flat ambient and emissive, and writes linear HDR (`R16G16B16A16_SFLOAT`).
3. **PostProcessPass** applies exposure and the Khronos PBR Neutral tonemapper into the swapchain image (taken from `FrameContext::targetImage`/`targetView`). If the swapchain format is not sRGB, it applies the sRGB encoding itself.
   - The swapchain *prefers* a UNORM format, so this is the normal path. ImGui's colors are already sRGB encoded; an sRGB swapchain would encode them again and wash the UI out.
4. **ImGuiPass** loads the image and draws ImGui's draw data over it with `ImGui_ImplVulkan_RenderDrawData`. It owns no GPU objects (see ImGui below).

How passes work together:
- **Layout transitions:** each pass does its own `pipelineBarrier2` in `BeginPass`/`EndPass`. Offscreen targets are shared by all frames in flight; the barrier at the start of each pass waits for the previous frame's readers.
  - The swapchain image stays in `eColorAttachmentOptimal` through every pass. `Renderer::Render` transitions it to `ePresentSrcKHR` after the last one, so a pass can be added at the end without moving that barrier.
- **Descriptor sets:** set 0 is always the global set (binding 0 camera, binding 1 lights) and every pipeline layout includes it. Set 1 holds the pass's inputs, which shaders read with `Texture2D.Load()`: sampled images, no samplers.
- **Per-draw data** goes in push constants (`GeometryPushConstants`: model matrix plus material factors, which must stay ≤128 bytes).
- **Shader modules** are loaded in `Renderer::SetupRenderPasses()` by constructing `Shader` objects directly, not through `ResourceManager`. The manager caches by id only, so the vertex and fragment stages of the same shader would collide in its cache.

### ImGui (`src/engine/UI/ImGuiLayer`)

- **Library:** Dear ImGui's docking branch, vendored as plain files in `src/external/ImGui` (core plus `backends/imgui_impl_glfw` and `imgui_impl_vulkan`, unmodified), built as the `imgui` static library. `CMakeLists.txt` records the upstream commit and how to update.
- **Ownership:** `ImGuiLayer` owns the context and both backends. It is created after the `Renderer` (its pipeline is built for the swapchain format) and waits for the GPU before shutting the backends down.
- **Panels:** the UI is made of `ImGuiPanel`s (`UI/ImGuiPanel.hpp`), each with a name, a visibility flag and a `Render()` that opens its own window with `ImGui::Begin(GetName().c_str(), GetVisibleFlag())`. Passing the flag gives the window a close button. `ImGuiLayer` is their manager:
  - `gui.AddPanel<T>(name, args...)` constructs `T(name, args...)`, like `RenderPassManager::AddRenderPass`. App adds its panels and menus in `App::BuildGui()`.
  - `gui.Render()` runs the whole ImGui frame: NewFrame, the main menu bar, a passthru dockspace over the rest of the main window, every *visible* panel in the order added, then `ImGui::Render()`. ImGui calls are only valid inside a panel's `Render()`.
  - App-specific panels live in `UI/Panels/` (`EnginePanel`: FPS and camera position; `DemoWindowPanel`: ImGui's demo window, hidden at start).
- **Menu bar:** `gui.GetMenuBar()` is an `ImGuiMenu` tree (`UI/ImGuiMenu.hpp`): `GetMenu(label)` finds or adds a submenu, `AddItem(label, action, { .shortcut, .enabled })` adds a command, `AddPanelItem(panel)` a check mark item that shows and hides a panel, and `AddSeparator()` a line.
  - The bar's submenus are its dropdown menus; an item added to the bar itself is a button on it.
  - The shortcut text is display only; bind the key with an event listener.
  - Commands run inside the ImGui frame, before the frame is drawn. Anything outside the UI should be changed by enqueuing an event (File > Quit enqueues `QuitApplicationEvent`), not directly.
  - Which panels are visible is not saved; `imgui.ini` only keeps window positions and docking.
- **Input comes from the bus**, not from GLFW callbacks: the GLFW backend is initialized with `install_callbacks = false`. Listeners at `ImGuiLayer::EventPriority` pass each event to the matching `ImGui_ImplGlfw_*Callback` and return `io.WantCaptureMouse`/`WantCaptureKeyboard` to swallow it. Releases, focus and enter/leave are never swallowed.
  - While the camera has the cursor captured, the layer ignores mouse input and sets `ImGuiConfigFlags_NoMouse`.
  - Platform windows the backend creates (panels dragged out of the main window) get the backend's own callbacks; their input never reaches the bus.
- **Multi-viewport** is on (`ImGuiLayer::EnableViewports`). Those windows have their own swapchains, which the backend renders and presents in `RenderPlatformWindows()` on the same queue.
- **Pipeline:** the backend's main-window pipeline is rebuilt only when the swapchain format changes, not on every resize. Rebuilding it after a platform window exists leaves the backend's viewport pipeline info pointing at freed memory (`imgui_impl_vulkan`, 1.93.1 WIP).
- **`imgui.ini`** is saved next to the executable (`FileSystem::getPath`).

### C++ ↔ shader contract

- **Shared structs** are mirrored in `Renderer/ShaderTypes.hpp` (`GpuLight`, `LightsUniform`, push constants), plus `CameraUniform` in `Camera/Camera.hpp`. Each mirror has `static_assert`s on offsets and size. Change both sides together.
  - Layout rule: every `vec3` is followed by a 4-byte scalar, so std140 and C++ agree without hidden padding.
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
  - `Scene::CreateEntity` initializes the entity immediately, so components added afterwards are initialized as they are added.
  - `Scene` owns materials behind `unique_ptr`, because `MeshComponent` holds raw `Material*` pointers.
- **Lights:** `LightComponent` uses glTF `KHR_lights_punctual` semantics and physical units: candela for point and spot lights, lux for directional. Exposure is 1.
  - The renderer collects lights from *all* entities each frame (lights are not culled), up to `MaxLights = 64`.
- **Culling:** `FrustumCulling` uses `MeshComponent::GetBoundingBox()`, which is still a fixed ±32 placeholder.
- **Resources:** `ResourceManager` caches resources by (type, id). A `Resource` implements `doLoad`/`doUnload`.
  - `Mesh` can be procedural: pass `MeshData` as the extra `Load` argument, e.g. `resources.Load<Mesh>("sphere", CreateSphere(...))` (see `Resource/Primitives.hpp`). Uploads need the transfer queue and pool in `ResourceContext`.
  - `HotReloadResourceManager` exists but is not used, and pipelines are not rebuilt when a shader reloads.
