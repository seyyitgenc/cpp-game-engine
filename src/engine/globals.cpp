#include "globals.h"
#include "callbacks.h"
#include <glad/glad.h>
#include <stb_image.h>
    
GLFWwindow *gWindow;

bool gEditModeEnabled;
bool gInitGlobals();
void InitShaders();
void InitTextures();
void InitCameras();
// Global variable initalization and backend setup

TextureManager   *gTextureManager   = nullptr;
ShaderManager    *gShaderManager    = nullptr;
CameraManager    *gCameraManager    = nullptr;

bool gInitGlobals() {
    Log::setLevel(Log::Debug);
    // glfw: initialize and configure
    // ------------------------------
    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit()){
        Log::write(Log::Fatal,"FATAL::INIT_GLOBALS Failed to init GLFW\n");
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    gWindow = glfwCreateWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"Engine", NULL, NULL);
    if (gWindow == NULL){
        Log::write(Log::Fatal,"FATAL::INIT_GLOBALS Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1); // on/off vscync
    
    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    
    glfwSetKeyCallback            (gWindow, Keyboard::keyCallback);
    setNormalModeCallbacks(gWindow);    
    // tell GLFW to capture our mouse
    glfwSetInputMode              (gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    gEditModeEnabled = false;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        Log::write(
            Log::Debug,
            "FATAL::GLAD_LOAD_GL_LOADER Failed to create GLFW window\n");
        return false;
    }
    // setup Dear ImGui context
    // ------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // setup Dear ImGui style
    // ----------------------
    ImGui::StyleColorsDark();

    // setup Platform/Renderer backends
    const char* glsl_version = "#version 460 core";
    ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    InitTextures();
    InitShaders();
    InitCameras();
    return true;
}
void InitTextures(){
    gTextureManager = TextureManager::getInstance(); 
    gTextureManager->addTexture("file-icon",FileSystem::getPath("resources/textures/file-icon.png"), TextureType::ICON);
}

void InitShaders(){
    gShaderManager = ShaderManager::getInstance();
    gShaderManager->addShader(
        "shader_model",
        FileSystem::getPath("shaders/basic_model.vs"),
        FileSystem::getPath("shaders/basic_model.fs"));
    gShaderManager->getShader("shader_model")->getShaderInfo().description = ("testing this thing .");
    gShaderManager->addShader(
        "shader_texture that has extremely long name but it is very very looooong name",
        FileSystem::getPath("shaders/basic_texture.vs"),
        FileSystem::getPath("shaders/basic_texture.fs"));
    gShaderManager->addShader(
        "shader_white_box",
        FileSystem::getPath("shaders/basic_mesh.vs"),
        FileSystem::getPath("shaders/white.fs"));
    gShaderManager->addShader(
        "shader_red_box",
        FileSystem::getPath("shaders/basic_mesh.vs"),
        FileSystem::getPath("shaders/red.fs"));
    gShaderManager->addShader(
        "shader_point_light",
        FileSystem::getPath("shaders/lighting/light.vs"),
        FileSystem::getPath("shaders/lighting/point_light.fs"));
    gShaderManager->addShader(
        "shader_spot_light",
        FileSystem::getPath("shaders/lighting/light.vs"),
        FileSystem::getPath("shaders/lighting/spot_light.fs"));
    gShaderManager->addShader(
        "shader_directional_light",
        FileSystem::getPath("shaders/lighting/light.vs"),
        FileSystem::getPath("shaders/lighting/directional_light.fs"));
    gShaderManager->addShader(
        "shader_light_cube",
        FileSystem::getPath("shaders/basic_mesh.vs"),
        FileSystem::getPath("shaders/color.fs"));
    gShaderManager->addShader(
        "shader_gbuffer",
        FileSystem::getPath("shaders/gbuffer.vs"),
        FileSystem::getPath("shaders/gbuffer.fs"));
    gShaderManager->addShader(
        "shader_lighting_pass",
        FileSystem::getPath("shaders/lighting/lighting_pass.vs"),
        FileSystem::getPath("shaders/lighting/lighting_pass.fs"));
    gShaderManager->addShader(
        "shader_depth_pass",
        FileSystem::getPath("shaders/lighting/depth_pass.vs"),
        FileSystem::getPath("shaders/lighting/depth_pass.fs"));
    gShaderManager->addShader(
        "shader_debugging",
        FileSystem::getPath("shaders/debugging.vs"),
        FileSystem::getPath("shaders/debugging.fs"));
    gShaderManager->addShader(
        "shader_debug_depth_pass",
        FileSystem::getPath("shaders/lighting/debug_depth_pass.vs"),
        FileSystem::getPath("shaders/lighting/debug_depth_pass.fs"));
}
void InitCameras(){
    gCameraManager = CameraManager::getInstance();
    gCameraManager->addCamera("scene_cam",glm::vec3{0.0f,0.0f,2.0f});
    gCameraManager->addCamera("test_cam",glm::vec3{1.0f,1.0f,1.0f});
    gCameraManager->addCamera("test_cam2",glm::vec3{-1.0f,-1.0f,-1.0f});
    gCameraManager->addCamera("test_cam3",glm::vec3{-4.0f,0.0f,5.0f});
    gCameraManager->addCamera("test_cam4",glm::vec3{-3.0f,2.0f,2.0f});
}