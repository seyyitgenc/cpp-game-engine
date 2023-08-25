#include "globals.h"
#include "callbacks.h"

GLFWwindow *gWindow;


// Global variable initalization
bool initGlobals() {
    
    // glfw: init and configure
    // ------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    gWindow = glfwCreateWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"Game Engine",NULL,NULL);
    if (gWindow == NULL)
    {
        std::cout << "ERROR::GLFW FAILED TO LOAD GLFW WINDOW" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(gWindow);
    glfwSetFramebufferSizeCallback(gWindow,framebuffer_size_callback);
    glfwSetCursorPosCallback(gWindow,mouse_callback);
    glfwSetScrollCallback(gWindow,scroll_callback);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "ERROR::GLAD FAILED TO INIT GLAD" << std::endl;
        return false;
    }
    
    return true;
}

