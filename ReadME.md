# GNC Engine

A C++23 game engine with a Vulkan 1.3 renderer and a Dear ImGui editor. It is being ported from OpenGL to Vulkan, loosely following the Khronos Vulkan Tutorial's "Building a Simple Engine" series. The old OpenGL engine lives on the `opengl-develop` branch.

> **Note:** for now the engine builds and runs on Windows only.

## Features

### Renderer

- Vulkan 1.3 with dynamic rendering and synchronization2, written with Vulkan-Hpp's RAII types
- Deferred shading: a G-buffer pass, a fullscreen lighting pass and a post-process pass
- Physically based shading: Cook-Torrance BRDF with glTF metallic-roughness materials
- Directional, point and spot lights in physical units (lux, candela), after glTF's `KHR_lights_punctual`; up to 64 per scene
- Exposure and the Khronos PBR Neutral tonemapper
- Frustum culling
- Shaders written in Slang and compiled to SPIR-V at build time
- Vulkan validation layers in Debug builds

### Editor

- Dockable panels, which can also be dragged out into their own windows
- Viewport panel: the scene, rendered at the panel's size
- Click an object in the Viewport to select it (GPU picking, exact to the pixel); the selection and the object under the cursor are outlined
- Move, rotate and scale the selection with a gizmo (ImGuizmo), in global or local space
- Scene panel: the entities and their components
- Inspector: edits the selected entity's components
- Create and delete entities, and add or remove components, from menus
- Engine panel: FPS and camera position

### Core

- Entity-component system; components describe their properties once, and the Inspector edits them from that
- Event bus with listener priorities; the UI sees input first and can keep it from the scene
- Resource manager, procedural meshes (sphere, plane)
- Fly camera
- Colored logger

## Requirements

- [Vulkan SDK](https://vulkan.lunarg.com/) 1.3 or newer, with the `VULKAN_SDK` environment variable set. It provides `slangc` and the validation layers.
- A GPU and driver with Vulkan 1.3
- [MSYS2](https://www.msys2.org/) with the MinGW64 toolchain and libraries:

      pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-glfw mingw-w64-x86_64-glm mingw-w64-x86_64-assimp

- A `make` for CMake to drive. MSYS2's `mingw-w64-x86_64-make` installs it as `mingw32-make`; pass `-G "MinGW Makefiles"` to the configure step to use it.

Dear ImGui (docking branch), ImGuizmo, stb_image, Font Awesome and IconFontCppHeaders are included in `src/external`.

## Build and run

From Git Bash:

    export PATH="/c/msys64/mingw64/bin:$PATH"
    cmake -S . -B build                      # Debug; add -DCMAKE_BUILD_TYPE=Release for a release build
    cmake --build build -j8
    ./run.sh                                 # or: cd bin/debug && ./engine.exe

The executable, compiled shaders and fonts go to `bin/debug/` (or `bin/release/`). Re-run the configure step after adding a `.cpp` or `.slang` file: sources are collected with `file(GLOB)`.

## Controls

| Input | Action |
|---|---|
| Hold right mouse | Look around |
| W A S D, while holding right mouse | Fly |
| E or Space / Q or Left Ctrl, while holding right mouse | Up / down |
| Left Shift, while holding right mouse | Fly faster |
| Mouse wheel | Zoom |
| Left click in the Viewport | Select an object |
| W / E / R | Move / rotate / scale gizmo |
| Delete | Delete the selected entity |
| Esc | Quit |

## TODO

### Rendering

- [ ] Model loading (glTF)
- [ ] Material textures
- [ ] Image based lighting, instead of the flat ambient term
- [ ] Shadow mapping
- [ ] Real bounding boxes for culling (meshes use a fixed placeholder box)
- [ ] Alpha modes (mask, blend)
- [ ] Instanced rendering
- [ ] G-buffer debug views
- [ ] Shader hot reloading (pipelines are not rebuilt yet)
- [ ] GPU timings per render pass

### Editor

- [ ] Gizmo snapping
- [ ] Transform parenting in the Scene panel
- [ ] Undo / redo
- [ ] Viewport icons for lights and other entities without a mesh, so they can be clicked
- [ ] Camera control in a Viewport dragged out into its own window

### Engine

- [ ] Editor / Engine hierarchy
  - [ ] Engine as a library, linked by an editor and a game executable (`GNC_EDITOR` define)
  - [ ] Leave the editor out of game builds: ImGui panels, picking and its entity id target, gizmos
  - [ ] Scene files, saved and loaded through the property system
  - [ ] Asset cooking: glTF to binary meshes, textures to BC7 (KTX2)
  - [ ] Pack file for the cooked assets, read through `FileSystem`
- [ ] Free materials no entity uses any more (deleting an entity keeps its material)
- [ ] Third person camera
- [ ] Scope stopwatch
