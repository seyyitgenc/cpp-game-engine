# GNC Engine

A C++23 game engine with a Vulkan 1.3 renderer and a Dear ImGui editor. It is being ported from OpenGL to Vulkan, loosely following the Khronos Vulkan Tutorial's "Building a Simple Engine" series. The old OpenGL engine lives on the `opengl-develop` branch.

> **Note:** for now the engine builds and runs on Windows only.

## Features

### Renderer

- Vulkan 1.3 with dynamic rendering and synchronization2, written with Vulkan-Hpp's RAII types
- Deferred shading: a G-buffer pass, a fullscreen lighting pass and a post-process pass
- Physically based shading: Cook-Torrance BRDF with glTF metallic-roughness materials
- Material textures: base color, metallic-roughness, normal, occlusion and emissive maps, mipmapped and anisotropically filtered
- Alpha-masked and double-sided materials
- Directional, point and spot lights in physical units (lux, candela), after glTF's `KHR_lights_punctual`; up to 64 per scene
- Exposure and the Khronos PBR Neutral tonemapper
- Sprites: unlit, alpha blended, drawn back to front by layer and then by depth, batched by texture; laid over the tonemapped scene, so they show their own colors, and hidden behind meshes in front of them
- Frustum culling against a bounding volume component on each entity: a box, a sphere, a capsule or a convex hull, fitted to its mesh
- Shaders written in Slang and compiled to SPIR-V at build time
- Vulkan validation layers in Debug builds

### Editor

- Dockable panels, which can also be dragged out into their own windows
- Viewport panel: the scene, rendered at the panel's size
- Click an object in the Viewport to select it (GPU picking, exact to the pixel): the first click selects the whole model, the next one the part. The selection and what a click would select are outlined.
- Move, rotate and scale the selection with a gizmo (ImGuizmo), in global or local space
- Editor grid on the ground plane (y = 0), or on z = 0 in 2D scenes, with the axes colored; hidden behind objects, fading with distance (the Viewport toolbar or View > Grid)
- Show the bounding volumes as wireframe, faint where the scene hides them (the Viewport toolbar or View > Bounding Volumes)
- Orientation gizmo in the Viewport's corner: shows the world axes, and a click on one turns the view to look along it, around the selection
- Entity hierarchy: transforms are relative to the parent, and deleting or deactivating an entity takes its children with it
- Scene panel: the hierarchy, with each entity's components
- Inspector: edits the selected entity's components
- Create and delete entities, and add or remove components, from menus
- Import models with assimp (glTF, OBJ, FBX, Collada, ...) from File > Import Model, as one entity tree: meshes, materials, textures and lights
- Import images as sprites from File > Import Sprite; a sprite is clicked and outlined by its visible pixels
- Scenes saved as `.gncscene` files (JSON, see [docs/SceneFormat.md](docs/SceneFormat.md)) and opened from the File menu; new scenes are 2D or 3D
- 2D scenes get a 2D editor: an orthographic view that pans and zooms, and a gizmo kept to the XY plane
- Engine panel: FPS and camera position

### Core

- Entity-component system; components describe their properties once, and the Inspector and scene files work from that
- Camera component (perspective or orthographic, after glTF's camera); the scene names the one it is seen through
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

Dear ImGui (docking branch), ImGuizmo, nlohmann/json, stb_image, Font Awesome and IconFontCppHeaders are included in `src/external`.

## Build and run

From Git Bash:

    export PATH="/c/msys64/mingw64/bin:$PATH"
    cmake -S . -B build                      # Debug; add -DCMAKE_BUILD_TYPE=Release for a release build
    cmake --build build -j8
    ./run.sh                                 # or: cd bin/debug && ./engine.exe

The executable, compiled shaders, fonts, models and textures go to `bin/debug/` (or `bin/release/`). Every folder under `resources/objects` that holds a `.gltf`, `.glb`, `.obj`, `.fbx` or `.dae` is copied to `models/` there, and File > Import Model lists every file in it that assimp reads. Every `.png`, `.jpg`, `.jpeg`, `.bmp` and `.tga` under `resources/textures` is copied to `textures/`, and File > Import Sprite lists them. Re-run the configure step after adding a `.cpp`, `.slang`, model or texture file: they are collected with `file(GLOB)`.

Scenes are not copied: the editor opens and saves the `.gncscene` files in `resources/scenes` in place, and starts with `Showcase.gncscene`.

`backpack.obj` (from LearnOpenGL) stores its texture coordinates upside down compared with other OBJ files, so its textures come out scrambled. `cyborg.blend` fails in assimp's Blender importer; import `cyborg.obj` instead.

## Controls

| Input | Action |
|---|---|
| Hold right mouse | Look around (2D scenes: drag the view) |
| W A S D, while holding right mouse | Fly (2D scenes: pan) |
| E or Space / Q or Left Ctrl, while holding right mouse | Up / down |
| Left Shift, while holding right mouse | Fly faster |
| Mouse wheel | Zoom (2D scenes: zoom the orthographic view) |
| Left click in the Viewport | Select a model; click again to select the part |
| W / E / R | Move / rotate / scale gizmo |
| Delete | Delete the selected entity |
| Ctrl+S | Save the scene |
| Click an axis of the orientation gizmo | View from that side; again for the opposite side |
| Esc | Quit |

## TODO

### Rendering

- [ ] Image based lighting, instead of the flat ambient term
- [ ] Shadow mapping
- [ ] Alpha blending (glTF BLEND materials draw opaque)
- [ ] MikkTSpace tangents for meshes that come without them
- [ ] Instanced rendering
- [ ] G-buffer debug views
- [ ] Shader hot reloading (pipelines are not rebuilt yet)
- [ ] GPU timings per render pass
- [ ] 2D
  - [x] Sprite component and a batched sprite pass: alpha blended, drawn in a sort order (layer, then z)
  - [ ] Sprite sheets and frame animation
  - [ ] Text rendering: MSDF font atlas, word wrap and alignment in a box, inline icons and colored keywords
  - [ ] Sprite effects: tint, outline, dissolve
  - [ ] 2D particles

### Editor

- [ ] Gizmo snapping
- [ ] Reparent entities by dragging them in the Scene panel
- [ ] Undo / redo
- [ ] Viewport icons for lights and other entities without a mesh, so they can be clicked
- [ ] Camera control in a Viewport dragged out into its own window
- [ ] Ask to save unsaved changes before opening another scene or quitting
- [ ] Remember the editor camera per scene and per user, instead of starting at the scene's camera
- [ ] Choose the scene's camera, and edit the scene's environment (ambient light, exposure), in the editor
- [ ] A material picker for mesh slots
- [ ] 2D scenes: 2D components (sprite, 2D camera) in the menus, zoom towards the cursor

### Engine

- [ ] Editor / Engine hierarchy
  - [ ] Engine as a library, linked by an editor and a game executable (`GNC_EDITOR` define)
  - [ ] Leave the editor out of game builds: ImGui panels, picking and its entity id target, gizmos
  - [x] Scene files, saved and loaded through the property system
  - [ ] Play mode that renders through the scene's camera
  - [ ] Asset cooking: glTF to binary meshes, textures to BC7 (KTX2)
  - [ ] Pack file for the cooked assets, read through `FileSystem`
- [ ] Free materials no entity uses any more (deleting an entity keeps its material)
- [ ] Model import: skins, animations, `KHR_texture_transform`, a second UV set, separate metalness and roughness maps
- [ ] Load textures on worker threads (a big import stalls the editor)
- [ ] Colliders
  - [ ] Collider component (box, sphere, capsule) with its own stored center, size and axis, upright by default and not refitted when the mesh changes
  - [ ] "Fit to mesh" as a first guess from the mesh's box: bottom at its base, centered on the box, the radius left to the user
  - [ ] A defined rule for non-uniform scale (a scaled capsule is no longer a capsule)
  - [ ] Collision tests between colliders
- [ ] Third person camera
- [ ] Scope stopwatch
- [ ] Tweening: position, rotation, scale and color over time with easing curves; sequences and callbacks
- [ ] Game UI, apart from ImGui (which stays the editor's)
  - [ ] Hover, click and drag and drop on sprites, hit tested on their bounds (the editor's picking is left out of game builds)
  - [ ] Buttons, panels, tooltips
  - [ ] Layout anchored to the screen, following the window size
- [ ] Audio (miniaudio): sound effects and music, volume groups
- [ ] Lua scripting (Lua 5.4 + sol2)
  - [ ] Scripts loaded through `FileSystem`, sandboxed (no `io`, `os`)
  - [ ] Coroutines that wait for the game: a player's choice, an animation
  - [ ] Hot reload without restarting
  - [ ] Script errors logged with the file and line, never a crash
- [ ] Networking
  - [ ] Transport (ENet or GameNetworkingSockets): reliable, ordered messages between a host and clients
  - [ ] Message serialization with a protocol version
  - [ ] Lobby: host, join by code or address, ready up
  - [ ] Steam lobbies and relay, so nobody has to forward ports
- [ ] Seeded random number generator with independent streams
- [ ] Save files: settings, an unfinished run

### Game

A roguelike deck-builder for friends: each player plays their own run at the same time, and players sabotage each other to get ahead.

- [ ] Paper prototype with friends, before any code
- [ ] Rules core, in C++ apart from the engine and the ECS
  - [ ] Game state: players, zones (draw pile, hand, discard, exhaust), cards, statuses, HP, energy, gold
  - [ ] Actions in, events out; every action validated against the state
  - [ ] Effect queue that resolves effects in order and fires triggers
  - [ ] Deterministic: a seeded stream per player and per system (map, shuffles, rewards, events), so one inserted card does not change later rolls
  - [ ] Action log: a seed and its log replay a game
  - [ ] Console client, playable over the network before any art
- [ ] Lua content API
  - [ ] Verbs: draw, deal damage, block, apply a status, gain energy or gold, add, remove or exhaust cards, send cards to a player, choose a card or target
  - [ ] Triggers for relics and statuses: turn start and end, card played, damage taken, card received from a player
  - [ ] Simple cards as JSON data built from these verbs; Lua for the rest
  - [ ] Determinism rules for scripts: no `math.random` or `os.time`, no `pairs()` over string keys in game logic (Lua 5.4 seeds string hashes per run)
- [ ] Run structure
  - [ ] Map generated per act from the seed: a node graph of fights, elites, shops, rest sites, events and the boss
  - [ ] Fights: enemies with intents, turns, rewards (a card choice, gold, relics)
  - [ ] Shops, rest sites (heal or upgrade), events with text and choices
  - [ ] Card upgrades and card removal
- [ ] Multiplayer
  - [ ] Parallel runs, one per player, simulated by the host; clients send actions and receive events
  - [ ] Hidden information: hands and draw piles never leave the host
  - [ ] Sabotage queued and applied at a safe point: the start of the target's next turn, fight or map step
  - [ ] The others' progress shown: floor, HP, gold, deck size
  - [ ] Win condition (first to beat the boss, last alive, or best score; to decide)
  - [ ] Reconnect from the seed and the action log
- [ ] Sabotage
  - [ ] Sabotage cards: junk cards into a deck, enemy buffs, reward theft, gold tax, map fog
  - [ ] Counterplay: block, reflect, cleanse
  - [ ] Catch-up, so the leader is the natural target: sabotage costs more against a player who is behind, and being hit gives a resource
- [ ] Presentation
  - [ ] Cards drawn from a template: frame, art, cost, name, rules text with keyword icons
  - [ ] Hand: fanned out, zoom on hover, drag to play, targeting arrow
  - [ ] Cards animate between zones
  - [ ] Screens: map, fight (with enemy intents), rewards, shop, event, lobby
  - [ ] Notifications ("Ali sent you 2 Curses") and an effect when a sabotage lands
  - [ ] Sound and music
- [ ] Tools
  - [ ] Card editor: definition, text, art and Lua effect, with a live preview from the card template
  - [ ] Relic, enemy (HP, intent pattern), encounter and event editors
  - [ ] Map generation preview from a seed
  - [ ] In-game debug console: set the seed, add a card or relic, jump to a floor, send a sabotage, kill the enemy
  - [ ] Headless bot simulator: full runs, win rate per card and relic, time per floor, how often the leader at halfway wins
  - [ ] Replay viewer: step through a seed and its action log
- [ ] First playable: 2 players, 1 act (about 10 floors and a boss), about 30 cards, 5 enemies, 2 elites, 5 relics, 4 or 5 sabotage cards and 1 counter card
