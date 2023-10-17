#include "app.h"

#include "shader.h"
#include "filesystem.h"
#include "model.h"
#include "camera.h"

App* App::s_instance;

App::App() { }
App::~App() { this->clean(); }

// initialize objects that will be used by game
// --------------------------------------------
void App::initEntities() {
}

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// give me some plane vertices to draw with indices

std::vector<float> planeVertices = {
    // positions         // texcoords
    5.0f, -0.5f, 5.0f,    1.0f, 0.0f,
    -5.0f, -0.5f, 5.0f,   0.0f, 0.0f,
    -5.0f, -0.5f, -5.0f,  0.0f, 1.0f,
    5.0f, -0.5f, -5.0f,   1.0f, 1.0f
};

// std::vector<float> planeVertices = {
//     // positions          // normals         // texcoords
//     5.0f, -0.5f, 5.0f,    0.0f, 1.0f, 0.0f,   5.0f, 0.0f,
//     -5.0f, -0.5f, 5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
//     -5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
//     5.0f, -0.5f, -5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 5.0f
// };


std::vector<unsigned int> planeIndices = {
    0, 1, 2,
    0, 2, 3
};

std::vector<float> cubeVertices = {
      // positions
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

      1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

void App::run() {
    initGlobals();


    std::vector<std::pair<std::string,std::string>> planeTextures;
    planeTextures.push_back(std::pair("bricks2.png","texture_diffuse"));
    planeTextures.push_back(std::pair("brickwall_normal.jpg","texture_normal"));
    Model plane(planeVertices,planeIndices,planeTextures);
    Model cube(cubeVertices);
    Model cyborg(FileSystem::getPath("bin/resources/objects/cyborg/cyborg.obj"));

    Shader modelShader(
        FileSystem::getPath("bin/shaders/basic_model.vs").c_str(),
        FileSystem::getPath("bin/shaders/basic_model.fs").c_str());
    
    Shader planeShader(
        FileSystem::getPath("bin/shaders/basic_texture.vs").c_str(),
        FileSystem::getPath("bin/shaders/basic_texture.fs").c_str());

    Shader lightCubeShader(
        FileSystem::getPath("bin/shaders/light_cube.vs").c_str(),
        FileSystem::getPath("bin/shaders/light_cube.fs").c_str());

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    glEnable(GL_DEPTH_TEST);
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

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::Begin("Debug Window");                           // Create a window called "Debug Window" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glClearColor(clear_color.x / clear_color.w, clear_color.y / clear_color.w, clear_color.z / clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            modelShader.use();
            // view/projection transformations
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", camera.GetViewMatrix());

                // // render the loaded model
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
                modelShader.setMat4("model", model);
                cyborg.Draw(modelShader);

                lightCubeShader.use();
                lightCubeShader.setMat4("projection", projection);
                lightCubeShader.setMat4("view", camera.GetViewMatrix());
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-1.0f, 4.0f, -5.0));
                model = glm::scale(model, glm::vec3(0.2f));
                lightCubeShader.setMat4("model", model);
                cube.Draw(lightCubeShader);

                planeShader.use();
                planeShader.setMat4("projection", projection);
                planeShader.setMat4("view", camera.GetViewMatrix());
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0));
                model = glm::scale(model, glm::vec3(2.0f));
                planeShader.setMat4("model", model);
                plane.Draw(planeShader);

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