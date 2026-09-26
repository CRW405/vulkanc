#include "glfwEngine.h"
#include "vEngine.h"

#include <stdio.h>

// -----------------------------------------------------------------------------
// GLFW window setup
// Graphics Library FrameWork allows us to create windows, handle inputs and
// events, and create OpenGL and Vulkan contexts.
//
// Make an empty window. Tell GLFW, "Do not use OpenGL. Vulkan will draw here."
// -----------------------------------------------------------------------------

bool glfwEngineCreate(glfwEngine *engine) {
	if (engine == NULL) {
		fprintf(stderr, "Failed to create GLFW engine: invalid engine state\n");
		return false;
	}

	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHintString(GLFW_WAYLAND_APP_ID, "make-me-float");

	engine->window = glfwCreateWindow(VENGINE_WINDOW_WIDTH, VENGINE_WINDOW_HEIGHT, "Hello World", NULL, NULL);
	if (engine->window == NULL) {
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		return false;
	}

	return true;
}

void glfwEngineDestroy(glfwEngine *engine) {
	if (engine == NULL) {
		return;
	}

	if (engine->window != NULL) {
		glfwDestroyWindow(engine->window);
	}
	glfwTerminate();
	*engine = (glfwEngine){ 0 };
}
