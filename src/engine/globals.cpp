#include "globals.h"
#include "callbacks.h"
#include <glad/glad.h>
#include <stb_image.h>
#include "util/log.h"

GLFWwindow *gWindow;
bool gEditModeEnabled;
void gInitShaders();
bool gInitGlobals();
ShaderManager *gShaderManager;

// Global variable initalization and backend setup

bool gInitGlobals() {
    CLog::setLevel(CLog::Debug);
    // glfw: initialize and configure
    // ------------------------------
    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit()){
        std::cout << "Failed to init glfw" << std::endl;
        return false;
    }
    

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    gWindow = glfwCreateWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"Engine", NULL, NULL);
    if (gWindow == NULL) {
        CLog::write(CLog::Fatal,"Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1); // on/off vscync
    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetCursorPosCallback      (gWindow, mouse_callback);
    glfwSetScrollCallback         (gWindow, scroll_callback);
    
    // tell GLFW to capture our mouse
    glfwSetInputMode              (gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    gEditModeEnabled = false;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    // setup Dear ImGui context
    // ------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    // setup Dear ImGui style
    // ----------------------
    ImGui::StyleColorsDark();

    // setup Platform/Renderer backends
    const char* glsl_version = "#version 460 core";
    ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    gInitShaders();
    return true;
}

void gInitShaders(){
    gShaderManager = ShaderManager::getInstance();
    gShaderManager->add_shader(
        "shader_model",
        FileSystem::getPath("shaders/basic_model.vs"),
        FileSystem::getPath("shaders/basic_model.fs"));
    gShaderManager->add_shader(
        "shader_texture",
        FileSystem::getPath("shaders/basic_texture.vs"),
        FileSystem::getPath("shaders/basic_texture.fs"));
    gShaderManager->add_shader(
        "shader_ray",
        FileSystem::getPath("shaders/ray.vs"),
        FileSystem::getPath("shaders/ray.gs"),
        FileSystem::getPath("shaders/ray.fs"));
    gShaderManager->add_shader(
        "shader_white_box",
        FileSystem::getPath("shaders/basic_mesh.vs"),
        FileSystem::getPath("shaders/white.fs"));
    gShaderManager->add_shader(
        "shader_red_box",
        FileSystem::getPath("shaders/basic_mesh.vs"),
        FileSystem::getPath("shaders/red.fs"));
}