#include "app.h"
#include "model.h"
#include "camera.h"
#include "shader.h"
#include "gui/gui.h"
#include "callbacks.h"
#include "shader_manager.h"
#include "../util/stopwatch.hpp"
#include "renderer/framebuffer.h"

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

        // Rendering
       
        // glClearColor(clear_color.x / clear_color.w, clear_color.y / clear_color.w, clear_color.z / clear_color.w, clear_color.w);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // view/projection transformations
    
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

        auto pos = glm::vec3(0,60,1);
            glm::mat4 projection = glm::perspective(glm::radians(camRef->Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = camRef->GetViewMatrix();
            glm::mat4 model = glm::mat4(1.0f);
        gBuffer.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            auto gAlbedoShader = gShaderManager->getShader("shader_albedo"); 
            gAlbedoShader->use();
            gAlbedoShader->setMat4("projection", projection);
            gAlbedoShader->setMat4("view", view);
            model = glm::mat4(1.0f);
            model = glm::translate(model,glm::vec3(0,0,0));
            model = glm::scale(model, glm::vec3(0.03f));
            gAlbedoShader->setMat4("model", model);
            sponza.Draw(*gAlbedoShader);
        gBuffer.unbind();
        // 2. lighting pass : calcualte lighting by itearationg over a screen filled quad pixel-by-pixel using the gbuffer's content
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto lightingPassShader = gShaderManager->getShader("shader_lighting_pass");

        lightingPassShader->use();

        // note: if i reload shader i must reload this aswell
        lightingPassShader->setInt("gPosition", 0);
        lightingPassShader->setInt("gNormal", 1);
        lightingPassShader->setInt("gAlbedoSpec", 2);

        gBuffer.bindTextures();

        lightingPassShader->setVec3("light.Position", pos);
        // lightingPassShader->setVec3("", pos);
        lightingPassShader->setVec3("light.Position", pos);
        lightingPassShader->setVec3("light.Color", glm::vec3(1));
        // update attenuation parameters and calculate radius
        const float linear = 0.009f;
        const float quadratic = 0.032f;
        lightingPassShader->setFloat("light.Linear", linear);
        lightingPassShader->setFloat("light.Quadratic", quadratic);
        lightingPassShader->setVec3("viewPos", camRef->Position);
        renderQuad();

        // 2.5 copy content of geometry's depth buffer to default framebuffer's depth buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer._framebuffer); // use g buffer to read
        glBindFramebuffer(GL_DRAW_BUFFER, 0); // write to default frame buffer
        glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (gShaderManager->bind("shader_light_cube")){
            gShaderManager->getShader("shader_light_cube")->setMat4("projection", projection);
            gShaderManager->getShader("shader_light_cube")->setMat4("view", camRef->GetViewMatrix());
            gShaderManager->getShader("shader_light_cube")->setVec3("color", {1,1,1});
            model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(0.5f));
            model = glm::translate(model, pos);
            gShaderManager->getShader("shader_light_cube")->setMat4("model", model);
            cube.Draw(*gShaderManager->getShader("shader_light_cube"));
            gShaderManager->unbind();
        }
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
// event loop
// ----------
void App::processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && !is_left_pressed){
        gCameraManager->setPrevCamera();
        is_left_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE){
        is_left_pressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && !is_right_pressed){
        gCameraManager->setNextCamera();
        is_right_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE){
        is_right_pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(gWindow,true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camRef->processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camRef->processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camRef->processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camRef->processKeyboard(RIGHT, deltaTime);
    
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        gShaderManager->reloadAllShaders();
    }

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && !is_j_pressed)
    {
        gEditModeEnabled = !gEditModeEnabled;
        is_j_pressed = true;
        if (gEditModeEnabled)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camRef->setLastMouse(true);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_RELEASE)
    {
        is_j_pressed = false;
    }
}

void App::update(const float& dt) {
}

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