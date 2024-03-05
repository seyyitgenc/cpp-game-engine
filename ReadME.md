# This project is all about OpenGL based C++ game engine

- **_NOTE:_**  For now, this project works on only linux.

> REQUIRED LIBS FOR OpenGL branch

- OpenGL, GLFW, GLM, Assimp, GLEW  

## Follow these steps after environment setup

    mkdir build
    cd build
    cmake .. -G "Ninja"
    make

- Change this -G parameter according to your build preferences

> TODO

- [x] ImGui
- ![ImGui](/screenshots/imgui.png)
- [x] Shaders
- [x] Hot Reloading Shaders
- ![Shaders](/screenshots/hot_reloading_shaders.png)
- [x] Camera
- [x] Multiple Cameras
- ![Camera](/screenshots/multiple_cameras.png)
- [x] Model loading
- ![Model](/screenshots/model_loading.png)
- [x] Logger(With colors)
- ![Log](/screenshots/colored_log.png)
- [x] Custom Model Loading
- [x] Stopwatch
- [ ] Different Materials (wood, metal, rock etc...)
- [ ] Directional Lighting
- [ ] Birectional Lighting
- [x] Multiple Point Lights (WIP)
- [ ] Simple Shadows
- [ ] Framebuffers (gBuffer, colorBuffer etc...)
- [ ] Debugging Framebuffers
- [ ] Deferred Shading
- [ ] Input Manager
- [ ] Editor / Engine hierarchy
- [ ] ECS
- [ ] Scene Hierarchy
- [ ] Scene Editor Tool
- [ ] Gizmos (translate/scale/rotate)
- [ ] Instanced Rendering
