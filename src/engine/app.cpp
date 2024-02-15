#include "app.h"
#include "util/log.h"
#include "shader.h"
#include "model.h"
#include "camera.h"
#include "callbacks.h"
#include "object_vertices.h"
#include "shader_manager.h"
#include "util/stopwatch.hpp"
#include "gui/gui.h"

App* App::s_instance;

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
    Model plane(planeVertices,planeIndices,planeTextures);
    // Model cube(cubeVertices);
    Model cyborg(FileSystem::getPath("resources/objects/cyborg/cyborg.obj"));

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    glEnable(GL_DEPTH_TEST);
    // glm::vec3 picking_pos = glm::vec3(0); //
    // note: must be refactored
    t1.start();
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
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f); // NOLINT
        glm::mat4 model = glm::mat4(1.0f);
    
        gShaderManager->bind("shader_model");
        // shader.bind();
        // todo : create function that sets these variables
        gShaderManager->getShader("shader_model").setMat4("projection", projection);
        gShaderManager->getShader("shader_model").setMat4("view", camera.GetViewMatrix());
        // render the loaded model
        model = glm::translate(model, glm::vec3(0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f));	// it's a bit too big for our scene, so scale it down
        gShaderManager->getShader("shader_model").setMat4("model", model);
        cyborg.Draw(gShaderManager->getShader("shader_model"));
        gShaderManager->unbind();

        // note : i can use it like this aswell
        gShaderManager->bind("shader_red_box");
        gShaderManager->getShader("shader_red_box").setMat4("projection", projection);
        gShaderManager->getShader("shader_red_box").setMat4("view", camera.GetViewMatrix());
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0));
        model = glm::scale(model, glm::vec3(2.0f));
        gShaderManager->getShader("shader_red_box").setMat4("model", model);
        plane.Draw(gShaderManager->getShader("shader_red_box"));
        gShaderManager->unbind();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(gWindow);
        glfwPollEvents();

    }
}
// event loop
// ----------
void App::processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(gWindow,true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
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