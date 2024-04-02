#include "app.h"
#include "model.h"
#include "camera.h"
#include "shader.h"
#include "gui/gui.h"
#include "callbacks.h"
#include "shader_manager.h"
#include "../util/stopwatch.hpp"

// todo: rename some of the functions of Camera and CameraMananger.

App* App::_instance = nullptr;

// fixme: this is not safe
Camera *camRef = nullptr;

App::App() { }
App::~App() { this->clean(); }
 
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
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
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
glm::vec3 lightPos(0,1,0);
// ---------
// main loop
// ---------
void App::run() {
    gInitGlobals();
    Model plane(FileSystem::getPath("resources/objects/TwoSidedPlane/glTF/TwoSidedPlane.gltf"));
    Model cube(FileSystem::getPath("resources/objects/Box/glTF-Binary/Box.glb"));
    Model camera(FileSystem::getPath("resources/objects/camera/10124_SLR_Camera_SG_V1_Iteration2.obj"));
    Model sponza(FileSystem::getPath("resources/objects/Sponza/glTF/Sponza.gltf"));
    Model cyborg(FileSystem::getPath("resources/objects/cyborg/cyborg.obj"));
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    glEnable(GL_DEPTH_TEST);

    gCameraManager->setActiveCamera(gCameraManager->getCamera("scene_cam"));
    
    FrameBuffer gBuffer;
    gBuffer.bind();
    gBuffer.attachRenderBuffer();
    gBuffer.attachTexture(SCREEN_WIDTH, SCREEN_HEIGHT, FBTT::POSITION, GL_COLOR_ATTACHMENT0);
    gBuffer.attachTexture(SCREEN_WIDTH, SCREEN_HEIGHT, FBTT::NORMAL, GL_COLOR_ATTACHMENT1);
    gBuffer.attachTexture(SCREEN_WIDTH, SCREEN_HEIGHT, FBTT::ALBEDO, GL_COLOR_ATTACHMENT2);
    gBuffer.checkCompleteness();
    gBuffer.unbind();

    FrameBuffer shadowMap;
    shadowMap.bind();
    shadowMap.attachRenderBuffer();
    shadowMap.attachTexture(SCREEN_WIDTH, SCREEN_HEIGHT, FBTT::SHADOW, GL_DEPTH_ATTACHMENT);
    shadowMap.checkCompleteness();
    shadowMap.unbind();
    
    // renderer instance
    _deferredRenderer = new DeferredRenderer(SCREEN_WIDTH, SCREEN_HEIGHT);

    while (!glfwWindowShouldClose(gWindow))
    {
        camRef = gCameraManager->getActiveCamera();

        // per-frame time logic
        // --------------------
        auto dts = t1.getElapsedTime<float>();
        deltaTime = dts;
        // std::cout << "dtMs : " << dts << std::endl;
        // update(deltaTime);
        t1.reset();

        processInput(gWindow);
        // show edit window

        glClearColor(clear_color.x / clear_color.w, clear_color.y / clear_color.w, clear_color.z / clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        auto modelShader = gShaderManager->getShader("shader_model");
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(camRef->Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camRef->GetViewMatrix();
            modelShader->use();
            model = glm::mat4(1.0f);
            model = glm::translate(model,glm::vec3(0,0,0));
            model = glm::scale(model, glm::vec3(0.03f));
            modelShader->setMat4("model", model);
            modelShader->setMat4("projection", projection);
            modelShader->setMat4("view", view);
            sponza.Draw(*modelShader);

        // render the cameras
        // for (auto &&i : *gCameraManager->getCameraList())
        // {
        //     if (i.second.get() != camRef)
        //     {
        //         if (gShaderManager->bind("shader_model"))
        //         {
        //             // todo : create function that sets these variables
        //             gShaderManager->getShader("shader_model")->setMat4("projection", projection);
        //             gShaderManager->getShader("shader_model")->setMat4("view", camRef->GetViewMatrix());
        //             model = glm::inverse(i.second->GetViewMatrix());

        //             model = glm::scale(model, glm::vec3(0.003f));	// it's a bit too big for our scene, so scale it down
        //             model = glm::translate(model, i.second.get()->Position);
                    
        //             // Create rotation matrices for pitch and yaw
        //             model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // these are for rotation correction
        //             model = glm::rotate(model,glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // these are for rotation correction
                    
        //             gShaderManager->getShader("shader_model")->setMat4("model", model);
        //             camera.Draw(*gShaderManager->getShader("shader_model"));
        //             gShaderManager->unbind();
        //         }
        //     }
        // }

        if (gEditModeEnabled){
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            Gui::Init();
            
            ImGui::ShowDemoWindow();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(gWindow);
        glfwPollEvents();

    }
}
// ----------
// event loop
// ----------
void App::processInput(GLFWwindow* window) {
    if (Keyboard::key(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(gWindow, true);

    if (!gEditModeEnabled){
        camRef->handleEvents(deltaTime);
    }
    gCameraManager->handleEvents(deltaTime);
    gShaderManager->handleEvents(deltaTime);
    
    Gui::handleEvents(deltaTime);
    if(Keyboard::keyWentDown(GLFW_KEY_J)){
        gEditModeEnabled = !gEditModeEnabled;
        if (gEditModeEnabled){
            glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            setEditModeCallbacks(gWindow);
        }
        else{
            glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            setNormalModeCallbacks(gWindow);
            Mouse::setFirstMouse(true);
        }
    }
}

// ------
// update
// ------
void App::update(const float& dt) {
}

// ------
// render
// ------
void App::render() {
    glClearColor(0.1f,0.1f,0.1f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // TODO: move rendering code here after simplifying it the render code
    // render stuff here

    glfwSwapBuffers(gWindow);
    glfwPollEvents();
}

void App::clean() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(gWindow);
    glfwTerminate();
}