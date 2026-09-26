#ifndef GLFW_ENGINE_H
#define GLFW_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdbool.h>

typedef struct glfwEngine {
	GLFWwindow *window;
} glfwEngine;

bool glfwEngineCreate(glfwEngine *engine);
void glfwEngineDestroy(glfwEngine *engine);

#endif
