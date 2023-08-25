#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <iostream>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "shader.h"
#include "filesystem.h"

int main() {


    // if (!glfwInit())
    //     return 1;

    // const char* glsl_version = "#version 430 core";
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    // // Setup Dear ImGui context
    // IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    // //ImGui::StyleColorsLight();

    // // Setup Platform/Renderer backends
    // ImGui_ImplGlfw_InitForOpenGL(window, true);
    // ImGui_ImplOpenGL3_Init(glsl_version);

  

    // // Our state
    // bool show_demo_window = true;
    // bool show_another_window = false;
    // ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
    //  Shader test(
    //     FileSystem::getPath("bin/shaders/triangle.vs").c_str(),
    //     FileSystem::getPath("bin/shaders/triangle.fs").c_str());

    // while (!glfwWindowShouldClose(window))
    // {
    //     glfwPollEvents();

    //     // Start the Dear ImGui frame
    //     ImGui_ImplOpenGL3_NewFrame();
    //     ImGui_ImplGlfw_NewFrame();
    //     ImGui::NewFrame();

    //     // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    //     if (show_demo_window)
    //         ImGui::ShowDemoWindow(&show_demo_window);

    //     // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    //     {
    //         static float f = 0.0f;
    //         static int counter = 0;
           

    //         ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    //         ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    //         ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    //         ImGui::Checkbox("Another Window", &show_another_window);

    //         ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    //         ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    //         if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
    //             counter++;
    //         ImGui::SameLine();
    //         ImGui::Text("counter = %d", counter);

    //         ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    //         ImGui::End();
    //     }

    //     // 3. Show another simple window.
    //     if (show_another_window)
    //     {
    //         ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    //         ImGui::Text("Hello from another window!");
    //         if (ImGui::Button("Close Me"))
    //             show_another_window = false;
    //         ImGui::End();
    //     }


    //     // Rendering
    //     ImGui::Render();
       
    //     int display_w, display_h;
    //     glfwGetFramebufferSize(window, &display_w, &display_h);
       
    //     glViewport(0, 0, display_w, display_h);
    //     glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    //     glClear(GL_COLOR_BUFFER_BIT);

    //     test.use();
    //     glBindVertexArray(VAO);
    //     glDrawArrays(GL_TRIANGLES,0,6);

    //     ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //     glfwSwapBuffers(window);
    // }
    // // Cleanup
    // ImGui_ImplOpenGL3_Shutdown();
    // ImGui_ImplGlfw_Shutdown();
    // ImGui::DestroyContext();

    // glfwDestroyWindow(window);
    // glfwTerminate();

    // return 0;
}
