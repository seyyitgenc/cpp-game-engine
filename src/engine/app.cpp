#include "app.h"
#include "util/log.h"
#include "shader.h"
#include "model.h"
#include "camera.h"
#include "callbacks.h"
#include "object_vertices.h"
#include "shader_manager.h"

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

void Triad_3D(){
}

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
    glm::vec3 picking_pos = glm::vec3(0);
    while (!glfwWindowShouldClose(gWindow))
    {
        processInput(gWindow);
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // start dear ImGui frame 
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // show demo window
        // ImGui::ShowDemoWindow();
        static float f0 = 0,f1 = 0,f2 = 0;
        static float colboxSizex = 0,colboxSizey = 0,colboxSizez = 0;
        static float colbox0 = 0, colbox1 = 0, colbox2 = 1;
        if (gEditModeEnabled)
        {
            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
            {
                static float f = 0.0f;
                static int counter = 0;
                ImGui::Begin("Debug Window");                           // Create a window called "Debug Window" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::Text("Object Location");               // for now this represents the cyborgs location
                ImGui::PushItemWidth(120);
                // note: imagine you have lots of objects you must list all of them
                // note: and can edit their location in the world.
                ImGui::SliderFloat("X", &colbox0, -10.0f, 10.0f); ImGui::SameLine();
                ImGui::SliderFloat("Y", &colbox1, -10.0f, 10.0f); ImGui::SameLine();
                ImGui::SliderFloat("Z", &colbox2, -10.0f, 10.0f);
                
                ImGui::Text("Object Size");
                ImGui::SliderFloat("w", &colboxSizex, -10.0f, 10.0f); ImGui::SameLine();
                ImGui::SliderFloat("h", &colboxSizey, -10.0f, 10.0f); ImGui::SameLine();
                ImGui::SliderFloat("d", &colboxSizez, -10.0f, 10.0f);

                ImGui::Text("Point Pos x: %.6f y: %.6f z: %.6f",picking_pos.x, picking_pos.y, picking_pos.z);
                ImGui::End();
            }
        }

        // Rendering
        ImGui::Render();
            glClearColor(clear_color.x / clear_color.w, clear_color.y / clear_color.w, clear_color.z / clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // view/projection transformations
                glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f);
                glm::mat4 model = glm::mat4(1.0f);
                Shader shader = gShaderManager->get_shader("shader_model");
                // gshader->bind_shader("shader_model");
                shader.bind();
                // todo : create function that sets these variables
                shader.setMat4("projection", projection);
                shader.setMat4("view", camera.GetViewMatrix());
                // render the loaded model
                model = glm::translate(model, glm::vec3(f0, f1, f2)); // translate it down so it's at the center of the scene
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
                shader.setMat4("model", model);
                cyborg.Draw(shader);
                shader.unbind();
                // gshader->bind_shader("shader_model");

                // lightCubeShader.use();
                // lightCubeShader.setMat4("projection", projection);
                // lightCubeShader.setMat4("view", camera.GetViewMatrix());
                // model = glm::mat4(1.0f);
                // model = glm::translate(model, glm::vec3(-1.0f, 4.0f, -5.0));
                // model = glm::scale(model, glm::vec3(0.2f));
                // lightCubeShader.setMat4("model", model);
                // cube.Draw(lightCubeShader);
                shader = gShaderManager->get_shader("shader_red_box");
                // note : i can use it like this aswell
                // gshader->bind_shader("shader_red_box");
                shader.bind();
                shader.setMat4("projection", projection);
                shader.setMat4("view", camera.GetViewMatrix());
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0));
                model = glm::scale(model, glm::vec3(2.0f));
                shader.setMat4("model", model);

                plane.Draw(shader);
                shader.unbind();
                // gshader->unbind_shader("shader_red_box");
                
                // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                // collisionBox.use();
                // collisionBox.setMat4("projection", projection);
                // collisionBox.setMat4("view", camera.GetViewMatrix());
                // model = glm::mat4(1.0f);
                // model = glm::translate(model, glm::vec3(colbox0,colbox1,colbox2));
                // model = glm::scale(model, glm::vec3(1.0f,1.0f,1.0f));
                // collisionBox.setMat4("model", model);
                // cube.Draw(collisionBox);
                // double x,y;
                // glfwGetCursorPos(gWindow,&x,&y);
                // glm::vec2 screen_pos(x,y);
                // screen_pos.y = SCREEN_HEIGHT - screen_pos.y;
                
                // glm::vec3 a(screen_pos.x,screen_pos.y,0);
                // glm::vec3 b(screen_pos.x,screen_pos.y,100);

                // glm::vec3 result = glm::unProject(a,camera.GetViewMatrix(),projection,glm::vec4(0,0,SCREEN_WIDTH,SCREEN_HEIGHT));
                // glm::vec3 result2 = glm::unProject(b,camera.GetViewMatrix(),projection,glm::vec4(0,0,SCREEN_WIDTH,SCREEN_HEIGHT));

                // picking_pos = result;
                // glm::vec3 picking_ray = result2 - result;

                // if (pointvsbox(picking_pos,glm::vec3(colbox0,colbox1,colbox2)))
                //     std::cout << "collided" << std::endl;
                // redBox.use();
                // redBox.setMat4("projection", projection);
                // redBox.setMat4("view", camera.GetViewMatrix());
                // model = glm::mat4(1.0f);
                // model = glm::translate(model, glm::vec3(-1.0f,2.0f,1.0f));
                // model = glm::scale(model, glm::vec3(0.1f));
                // redBox.setMat4("model", model);
                // cube.Draw(redBox);

                // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
        gShaderManager->reload_shaders();
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
    glClearColor(0.1f,0.1f,0.1f,1.0f);
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