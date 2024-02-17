#include "globals.h"
#include "callbacks.h"
#include <glad/glad.h>
#include <stb_image.h>
    
GLFWwindow *gWindow;
bool gEditModeEnabled;
void gInitShaders();
bool gInitGlobals();
void gInitTextures();
ShaderManager *gShaderManager;
// StopwatchManager *gStopwatchManager;

// Global variable initalization and backend setup

bool gInitGlobals() {
    Log::setLevel(Log::All);
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
        Log::write(Log::Fatal,"FATAL::INIT_GLOBALS Failed to create GLFW window\n");
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
        Log::write(Log::Fatal,"FATAL::GLAD_LOAD_GL_LOADER Failed to create GLFW window\n");
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
    gInitTextures();
    gInitShaders();
    return true;
}
void gInitTextures(){
    TextureManager::getInstance()->addTexture("file-icon",FileSystem::getPath("resources/textures/file-icon.png"), ICON);
}

void gInitShaders(){
    gShaderManager = ShaderManager::getInstance();
    gShaderManager->addShader(
        "shader_model",
        FileSystem::getPath("shaders/basic_model.vs"),
        FileSystem::getPath("shaders/basic_model.fs"));
    gShaderManager->addShader(
        "shader_texture",
        FileSystem::getPath("shaders/basic_texture.vs"),
        FileSystem::getPath("shaders/basic_texture.fs"));
    gShaderManager->addShader(
        "shader_raythisshaderhasextremelylongname",
        FileSystem::getPath("shaders/ray.vs"),
        FileSystem::getPath("shaders/ray.gs"),
        FileSystem::getPath("shaders/ray.fs"));
    gShaderManager->addShader(
        "shader_white_box",
        FileSystem::getPath("shaders/basic_mesh.vs"),
        FileSystem::getPath("shaders/white.fs"));
    gShaderManager->addShader(
        "shader_red_box",
        FileSystem::getPath("shaders/basic_mesh.vs"),
        FileSystem::getPath("shaders/red.fs"));
}