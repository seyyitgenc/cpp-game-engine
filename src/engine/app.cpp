#include "app.h"
#include "shader.h"
#include "model.h"
#include "camera.h"
#include "callbacks.h"
#include "object_vertices.h"
#include "shader_manager.h"
#include "util/stopwatch.hpp"
#include "gui/gui.h"

App* App::_instance = nullptr;
Camera* App::camRef = nullptr;

App::App() { }
App::~App() { this->clean(); }
 
// void init_shader_values(const std::string &name){
//     Shader shader = ShaderManager::getInstance()->get_shader(name);
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

Stopwatch<> t1; //nolint

void App::run() {
    gInitGlobals();
    std::vector<std::pair<std::string,std::string>> planeTextures;
    planeTextures.push_back(std::pair("bricks2.png","texture_diffuse"));
    planeTextures.push_back(std::pair("brickwall_normal.jpg","texture_normal"));
    Model plane(planeVertices,planeIndices);
    Model camera(FileSystem::getPath("resources/objects/camera/10124_SLR_Camera_SG_V1_Iteration2.obj"));
    // Model cube(cubeVertices);
    Model cyborg(FileSystem::getPath("resources/objects/cyborg/cyborg.obj"));

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    glEnable(GL_DEPTH_TEST);
    // glm::vec3 picking_pos = glm::vec3(0); //
    // note: must be refactored
    t1.start();
    
    camRef = CameraManager::getInstance()->getCamera("scene_cam");

    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame time logic
        // --------------------
        auto dtMs = t1.getElapsedTime<float>();
        deltaTime = dtMs;
        // std::cout << "dtMs : " << dtMs << std::endl;
        // update(deltaTime);
        t1.reset();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        processInput(gWindow);

        // show edit window
        if (gEditModeEnabled){
            Gui::Init();
        }
        // Rendering
        ImGui::Render();
       
        glClearColor(clear_color.x / clear_color.w, clear_color.y / clear_color.w, clear_color.z / clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camRef->Zoom), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f); // NOLINT
        glm::mat4 model = glm::mat4(1.0f);
    
        // render the cameras
        for (auto &&i : CameraManager::getInstance()->getCameraList())
        {
            if (i.second.get() != camRef)
            {
                model = glm::mat4(1.0f);
                ShaderManager::getInstance()->bind("shader_model");
                // todo : create function that sets these variables
                ShaderManager::getInstance()->getShader("shader_model").setMat4("projection", projection);
                ShaderManager::getInstance()->getShader("shader_model").setMat4("view", camRef->GetViewMatrix());
                // // Convert pitch and yaw to radians
                // float pitchRadians = glm::radians(i.second.get()->Pitch);
                // float yawRadians = glm::radians(i.second.get()->Yaw);

                // // Create rotation matrices for pitch and yaw
                // glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), pitchRadians, glm::vec3(1.0f, 0.0f, 0.0f));
                // glm::mat4 yawMatrix = glm::rotate(glm::mat4(1.0f), yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));

                // // Combine the pitch and yaw rotations
                // glm::mat4 rotationMatrix = yawMatrix * pitchMatrix;

                // // Apply the rotation to the model matrix
                // model = rotationMatrix * model;
                model = glm::translate(model, i.second.get()->Position);
                model = glm::scale(model, glm::vec3(0.010f));	// it's a bit too big for our scene, so scale it down
                ShaderManager::getInstance()->getShader("shader_model").setMat4("model", model);
                camera.Draw(ShaderManager::getInstance()->getShader("shader_model"));
                ShaderManager::getInstance()->unbind();
            }
        }

        model = glm::mat4(1.0f);
        ShaderManager::getInstance()->bind("shader_model");
        // todo : create function that sets these variables
        ShaderManager::getInstance()->getShader("shader_model").setMat4("projection", projection);
        ShaderManager::getInstance()->getShader("shader_model").setMat4("view", camRef->GetViewMatrix());
        model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0));
        model = glm::scale(model, glm::vec3(1.0f));	// it's a bit too big for our scene, so scale it down
        ShaderManager::getInstance()->getShader("shader_model").setMat4("model", model);
        cyborg.Draw(ShaderManager::getInstance()->getShader("shader_model"));
        ShaderManager::getInstance()->unbind();

        // note : i can use it like this aswell
        ShaderManager::getInstance()->bind("shader_red_box");
        ShaderManager::getInstance()->getShader("shader_red_box").setMat4("projection", projection);
        ShaderManager::getInstance()->getShader("shader_red_box").setMat4("view", camRef->GetViewMatrix());
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0));
        model = glm::scale(model, glm::vec3(2.0f));
        ShaderManager::getInstance()->getShader("shader_red_box").setMat4("model", model);
        plane.Draw(ShaderManager::getInstance()->getShader("shader_red_box"));
        ShaderManager::getInstance()->unbind();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(gWindow);
        glfwPollEvents();

    }
}
// event loop
// ----------
void App::processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !is_n_pressed)
    {
        is_n_pressed = true;
        camRef = CameraManager::getInstance()->getCamera("test_cam");
        camRef->setLastMouse(true);

    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE)
    {
        is_n_pressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !is_m_pressed)
    {
        is_m_pressed = true;
        camRef = CameraManager::getInstance()->getCamera("scene_cam");
        camRef->setLastMouse(true);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE)
    {
        is_m_pressed = false;
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
        ShaderManager::getInstance()->reloadAllShaders();
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
    glClearColor(0.1f,0.1f,0.1f,1.0f); //nolint
    glClear(GL_COLOR_BUFFER_BIT);

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