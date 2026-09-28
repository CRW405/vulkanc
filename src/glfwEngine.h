#ifndef GLFW_ENGINE_H
#define GLFW_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdbool.h>

typedef struct glfwEngine {
	GLFWwindow *window;
	char title[256];
	int fpsLimit;
	double frameStartTime;
	double previousFrameTime;
	double deltaTime;
	double fpsWindowStartTime;
	unsigned int fpsFrameCount;
} glfwEngine;

typedef struct glfwEngineConfig {
	int width;
	int height;
	const char *title;
	// 0 disables frame limiting; a positive value caps frames per second.
	int fpsLimit;
} glfwEngineConfig;

bool glfwEngineCreate(glfwEngine *engine, const glfwEngineConfig *config);
void glfwEngineFrameStart(glfwEngine *engine);
double glfwEngineGetDeltaTime(const glfwEngine *engine);
void glfwEngineFrameEnd(glfwEngine *engine);
void glfwEngineDestroy(glfwEngine *engine);

#endif
