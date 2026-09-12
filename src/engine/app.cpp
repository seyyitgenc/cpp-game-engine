#include "glad/glad.h"

#include "app.hpp"

#include "callbacks.h"
#include "camera.h"
#include "globals.h"
#include "model.h"
#include "shader.h"
#include "shader_manager.h"

#include "util/stopwatch.hpp"

#include "gui/ImGuiLayerManager.hpp"
#include "gui/ShadersGUI.hpp"

#include <glm/gtc/type_ptr.hpp>

// todo: rename some of the functions of Camera and CameraMananger.

namespace GNC {

App* App::_instance = nullptr;

// fixme: this is not safe
Camera* camRef = nullptr;

App& App::instance()
{
    if (_instance == nullptr) {
        _instance = new App();
        Log::writeFormatted(
            Log::Info,
            GREEN_TEXT(
                "INFO::APP::GET_INSTANCE App Initialized with mem address -> %p")
                .c_str(),
            &_instance);
    }
    return *_instance;
}

App::App()
    : clear_color(ImVec4(0.45f, 0.55f, 0.60f, 1.00f))
{
}

App::~App()
{
    this->clean();
}

// void init_shader_values(const std::string &name){
//     Shader shader = gShaderManager->get_shader(name);
//     shader.bind();
//     // shader.setMat4("projection", projection);
//     shader.setMat4("view", camera.GetViewMatrix());
//     // render the loaded model
//     // model = glm::translate(model, glm::vec3(f0, f1, f2)); // translate it down so it's at the center of the scene
//     // model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
//     // shader.setMat4("model", model);
//     shader.unbind();
// }
// TODO :: implement model gshader

Stopwatch<> t1;

float deltaTime = 0; // note: temporary solution

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0) {

        // clang-format off
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // clang-format on

        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

float x = 30.0f, y = 60.0f, z = -7.0f;

// ---------
// main loop
// ---------
void App::run()
{
    gInitGlobals();

    ImGuiLayerManager::instance().addPanel("Shaders", new ShadersGUI);

    auto shaderManager = ShaderManager::instance();
    auto debugDepthPassShader = shaderManager->getShader("shader_debug_depth_pass");
    auto depthPassShader = shaderManager->getShader("shader_depth_pass");
    auto gBufferShader = shaderManager->getShader("shader_gbuffer");
    auto lightingPassShader = shaderManager->getShader("shader_lighting_pass");
    auto debugShader = shaderManager->getShader("shader_debugging");

    // Model plane(FileSystem::getPath("resources/objects/TwoSidedPlane/glTF/TwoSidedPlane.gltf"));
    Model cube(FileSystem::getPath("resources/objects/BoxTextured/glTF/BoxTextured.gltf"));
    // Model camera(FileSystem::getPath("resources/objects/camera/10124_SLR_Camera_SG_V1_Iteration2.obj"));
    // Model earth(FileSystem::getPath("resources/objects/earth/Earth_1_12756.glb"));

    // cube_transf[0].scale(1.0f, 3.0f, 1.0f);
    // cube_transf[0].translate(2.0f, 0.0f, 2.0f);

    // cube_transf[1].scale(1.0f, 5.0f, 1.0f);
    // cube_tra`nsf[1].translate(-2.0f, 0.0f, -2.0f);

    // cube_base_transf.translate(0.0f, -2.0f, 0.0f);
    // cube_base_transf.scale(20.0f, 1.0f, 20.0f);

    std::vector<glm::vec3> cubeScales = { glm::vec3(1.0f, 3.0f, 1.0f), glm::vec3(1.0f, 5.0f, 1.0f), glm::vec3(20.0f, 1.0f, 20.0f) };
    std::vector<glm::vec3> cubeTransfors = { glm::vec3(-2.0f, 0.0f, -2.0f), glm::vec3(2.0f, 0.0f, 2.0f), glm::vec3(0.0f, -2.0f, 0.0f) };

    glEnable(GL_DEPTH_TEST);

    CameraManager::instance()->setActiveCamera(CameraManager::instance()->getCamera("scene_cam"));

    // FrameBuffer shadowMap;
    // shadowMap.bind(GL_FRAMEBUFFER);
    // shadowMap.attachRenderBuffer();
    // shadowMap.attachTexture(SHADOW_WIDTH, SHADOW_HEIGHT, FBTT::SHADOW, GL_DEPTH_ATTACHMENT);
    // glDrawBuffer(GL_NONE);
    // glReadBuffer(GL_NONE);
    // shadowMap.checkCompleteness();
    // shadowMap.unbind();
    // // renderer instance
    // FrameBuffer gBuffer;
    // gBuffer.bind(GL_FRAMEBUFFER);
    // gBuffer.attachRenderBuffer();
    // gBuffer.attachTexture(gViewport._width, gViewport._height, FBTT::POSITION, GL_COLOR_ATTACHMENT0);
    // gBuffer.attachTexture(gViewport._width, gViewport._height, FBTT::NORMAL, GL_COLOR_ATTACHMENT1);
    // gBuffer.attachTexture(gViewport._width, gViewport._height, FBTT::ALBEDO, GL_COLOR_ATTACHMENT2);
    // gBuffer.attachTexture(gViewport._width, gViewport._height, FBTT::ROUGHNESS, GL_COLOR_ATTACHMENT3);
    // gBuffer.checkCompleteness();
    // gBuffer.unbind();

    // Track previous viewport size for resize detection
    int prevWidth = gViewport._width;
    int prevHeight = gViewport._height;

    while (!glfwWindowShouldClose(gWindow)) {

        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }
}

// ----------
// event loop
// ----------
void App::processInput([[maybe_unused]] GLFWwindow* window)
{
    if (Keyboard::key(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(gWindow, true);

    if (!gEditModeEnabled) {
        camRef->handleEvents(deltaTime);
    }

    CameraManager::instance()->handleEvents(deltaTime);
    ShaderManager::instance()->handleEvents(deltaTime);

    if (Keyboard::keyWentDown(GLFW_KEY_P)) {
        static bool perspectiveProjection = false;
        perspectiveProjection = !perspectiveProjection;
        if (perspectiveProjection) {
            CameraManager::instance()->getCamera("light_cam")->setProjectionMatrixAsPerspective(glm::radians(45.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, 1.0f, 1000.0f);
        } else {
            CameraManager::instance()->getCamera("light_cam")->setProjectionMatrixAsOrtho(-50.0f, 50.0f, -50.0f, 50.0f, 1.0f, 100.0f);
        }
    }
    if (Keyboard::key(GLFW_KEY_O))
        y += 0.5;
    if (Keyboard::key(GLFW_KEY_L))
        y -= 0.5;
    if (Keyboard::key(GLFW_KEY_K))
        x += 0.5;
    if (Keyboard::key(GLFW_KEY_SEMICOLON))
        x -= 0.5;
    if (Keyboard::key(GLFW_KEY_Q))
        z -= 0.5;
    if (Keyboard::key(GLFW_KEY_E))
        z += 0.5;

    // Gui::handleEvents(deltaTime);
    if (Keyboard::keyWentDown(GLFW_KEY_J)) {
        gEditModeEnabled = !gEditModeEnabled;
        if (gEditModeEnabled) {
            glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            setEditModeCallbacks(gWindow);
        } else {
            glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            setNormalModeCallbacks(gWindow);
            Mouse::setFirstMouse(true);
        }
    }
}

// ------
// update
// ------
void App::update([[maybe_unused]] const float& dt)
{
}

// ------
// render
// ------
void App::render()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // TODO: move rendering code here after simplifying it the render code
    // render stuff here

    glfwSwapBuffers(gWindow);
    glfwPollEvents();
}

void App::clean()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(gWindow);
    glfwTerminate();
}

} // namespace GNC