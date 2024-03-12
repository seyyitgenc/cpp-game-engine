#include "app.h"
#include "shader.h"
#include "model.h"
#include "camera.h"
#include "callbacks.h"
#include "shader_manager.h"
#include "util/stopwatch.hpp"
#include "gui/gui.h"

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

    auto pLightProp = std::make_unique<PointLightProperties>();
    auto sLightProp = std::make_unique<SpotLightProperties>();
    pLight = new Light<LightSpec::Point>(std::move(pLightProp));
    sLight = new Light<LightSpec::Spot>(std::move(sLightProp));
    // testLights();
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
        glm::mat4 projection = glm::perspective(glm::radians(camRef->Zoom), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 400.0f); // NOLINT
        glm::mat4 model = glm::mat4(1.0f);
    
        // render the cameras
        for (auto &&i : *gCameraManager->getCameraList())
        {
            if (i.second.get() != camRef)
            {
                if (gShaderManager->bind("shader_model"))
                {
                    // todo : create function that sets these variables
                    gShaderManager->getShader("shader_model")->setMat4("projection", projection);
                    gShaderManager->getShader("shader_model")->setMat4("view", camRef->GetViewMatrix());
                    model = glm::inverse(i.second->GetViewMatrix());

                    model = glm::scale(model, glm::vec3(0.003f));	// it's a bit too big for our scene, so scale it down
                    model = glm::translate(model, i.second.get()->Position);
                    
                    // Create rotation matrices for pitch and yaw
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // these are for rotation correction
                    model = glm::rotate(model,glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // these are for rotation correction
                    
                    gShaderManager->getShader("shader_model")->setMat4("model", model);
                    camera.Draw(*gShaderManager->getShader("shader_model"));
                    gShaderManager->unbind();
                }
            }
        }

        if (gShaderManager->bind("shader_model")){
            model = glm::mat4(1.0f);
            // todo : create function that sets these variables
            gShaderManager->getShader("shader_model")->setMat4("projection", projection);
            gShaderManager->getShader("shader_model")->setMat4("view", camRef->GetViewMatrix());
            model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0));
            model = glm::scale(model, glm::vec3(1.0f));	// it's a bit too big for our scene, so scale it down
            gShaderManager->getShader("shader_model")->setMat4("model", model);
            cyborg.Draw(*gShaderManager->getShader("shader_model"));
            gShaderManager->unbind();
        }
 
        // note : i can use it like this aswell
        // if (gShaderManager->bind("shader_model")){
        //     gShaderManager->getShader("shader_model")->setMat4("projection", projection);
        //     gShaderManager->getShader("shader_model")->setMat4("view", camRef->GetViewMatrix());
        //     model = glm::mat4(1.0f);
        //     model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0));
        //     model = glm::scale(model, glm::vec3(5.0f,1.0,5.0));
        //     gShaderManager->getShader("shader_model")->setMat4("model", model);
        //     plane.Draw(*gShaderManager->getShader("shader_model"));
        //     gShaderManager->unbind();
        // }

        // if (gShaderManager->bind("shader_light_cube")){
        //     gShaderManager->getShader("shader_light_cube")->setMat4("projection", projection);
        //     gShaderManager->getShader("shader_light_cube")->setMat4("view", camRef->GetViewMatrix());
        //     gShaderManager->getShader("shader_light_cube")->setVec3("color", sLight->getProperties<SpotLightProperties>()->color);
        //     model = glm::mat4(1.0f);
        //     model = glm::scale(model, glm::vec3(0.5f));
        //     model = glm::translate(model, sLight->getProperties<SpotLightProperties>()->position);
        //     gShaderManager->getShader("shader_light_cube")->setMat4("model", model);
        //     cube.Draw(*gShaderManager->getShader("shader_light_cube"));
        //     gShaderManager->unbind();
        // }
        // fixme: this light pass sucks. because i cast light for each mesh seperatlys. model must act like a complete mesh
        model = glm::mat4(1.0f);
        // todo : create function that sets these variables

        sLight->getProperties<SpotLightProperties>()->position = camRef->Position;
        sLight->getProperties<SpotLightProperties>()->direction = camRef->Front;
        sLight->setUniforms("shader_spot_light");
        gShaderManager->getShader("shader_spot_light")->setMat4("projection", projection);
        gShaderManager->getShader("shader_spot_light")->setMat4("view", camRef->GetViewMatrix());
        gShaderManager->getShader("shader_spot_light")->setVec3("viewPos", camRef->Position);
        model = glm::translate(model, glm::vec3(-4.0f, -2.0f, 0.0));
        model = glm::scale(model, glm::vec3(0.05f));	// it's a bit too big for our scene, so scale it down
        gShaderManager->getShader("shader_spot_light")->setMat4("model", model);
        sponza.Draw(*gShaderManager->getShader("shader_spot_light"));
        gShaderManager->unbind();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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