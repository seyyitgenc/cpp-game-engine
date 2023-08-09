#include "app.h"

#include "shader.h"

App* App::s_instance;

App::App() { }
App::~App() { this->clean(); }

// initialize objects that will be used by game
// --------------------------------------------
void App::initEntities() {
}

// main loop
// --------------
void App::run() {
    
    initGlobals();

    // float vertices[] = {
    //     // first triangle
    //     -0.9f, -0.5f, 0.0f,  // left 
    //     -0.0f, -0.5f, 0.0f,  // right
    //     -0.45f, 0.5f, 0.0f,  // top 
    //     // second triangle
    //     0.0f, -0.5f, 0.0f,  // left
    //     0.9f, -0.5f, 0.0f,  // right
    //     0.45f, 0.5f, 0.0f   // top 
    // }; 

    // unsigned int VBO, VAO;
    // glGenVertexArrays(1, &VAO);
    // glGenBuffers(1, &VBO);
    // glBindVertexArray(VAO);

    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);

    // glBindBuffer(GL_ARRAY_BUFFER, 0); 
    // glBindVertexArray(0); 
    Object openglTriangle;
    std::vector<Object> myObjects;
    myObjects.push_back(openglTriangle);
    Shader test("../shaders/triangle.vs","../shaders/triangle.fs");
    while (!glfwWindowShouldClose(gWindow))
    {
        processInput();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // glBindVertexArray(VAO);
        // glDrawArrays(GL_TRIANGLES, 0, 6);
        for (auto i = 0; i < myObjects.size(); i++)
        {
            myObjects[i].draw(test);
        }
        
        glfwSwapBuffers(gWindow);
        glfwPollEvents();;
        // update(dt);
        // render();
    }
}

// event loop
// ----------
void App::processInput() {
    if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(gWindow,true);
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
   glfwTerminate();
}