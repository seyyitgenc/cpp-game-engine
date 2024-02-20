#pragma once
#include "camera.h"
#include "camera_manager.h"
#include "app.h"
inline void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0,0,width,height);
}

// scroll callback for edit mode
inline void scroll_callback_edit(GLFWwindow *window, double xpos, double ypos)
{
}

// mouse position callback for edit mode
inline void mouse_callback_edit(GLFWwindow *window, double xpos, double ypos)
{
}

inline void scroll_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (!gEditModeEnabled){
		App::getCamera().processMouseScroll(xpos, ypos);
	}
  
}
inline void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (!gEditModeEnabled){

		App::getCamera().processMouseMovement(xpos, ypos);
	}
}

inline  void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
