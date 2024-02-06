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
- ![Shaders](/screenshots/imgui.png)
- [x] Shaders
- [x] Hot Reloading Shaders
- ![Shaders](/screenshots/hot_reloading_shaders.png)
- [x] Camera
- [x] Model loading
- ![Shaders](/screenshots/model_loading.png)
- [x] Custom Model Loading
- [ ] Editor / Engine hierarchy
- [ ] ECS
- [ ] Scene Hierarchy
- [ ] Scene Editor Tool
- [ ] Gizmos (translate/scale/rotate)
- [ ] Instanced Rendering
- [ ] Directional Lighting
- [ ] Birectional Lighting
- [ ] Multiple Point Lights
- [ ] Simple Shadows
