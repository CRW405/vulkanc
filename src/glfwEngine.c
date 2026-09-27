#include "glfwEngine.h"
#include "vEngine.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

// -----------------------------------------------------------------------------
// GLFW window setup
// Graphics Library FrameWork allows us to create windows, handle inputs and
// events, and create OpenGL and Vulkan contexts.
//
// Make an empty window. Tell GLFW, "Do not use OpenGL. Vulkan will draw here."
// -----------------------------------------------------------------------------

bool glfwEngineCreate(glfwEngine *engine, const glfwEngineConfig *config) {
	if (engine == NULL || config == NULL || config->width <= 0 || config->height <= 0 ||
	    config->title == NULL || config->fpsLimit < 0) {
		fprintf(stderr, "Failed to create GLFW engine: invalid engine state\n");
		return false;
	}

	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHintString(GLFW_WAYLAND_APP_ID, "make-me-float");

	engine->window = glfwCreateWindow(config->width, config->height, config->title, NULL, NULL);
	if (engine->window == NULL) {
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		return false;
	}
	strncpy(engine->title, config->title, sizeof(engine->title) - 1);
	engine->title[sizeof(engine->title) - 1] = '\0';
	engine->fpsLimit = config->fpsLimit;
	engine->fpsWindowStartTime = glfwGetTime();
	glfwSetWindowSize(engine->window, config->width, config->height);

	return true;
}

void glfwEngineFrameStart(glfwEngine *engine) {
	if (engine == NULL)
		return;
	engine->frameStartTime = glfwGetTime();
}

void glfwEngineFrameEnd(glfwEngine *engine) {
	if (engine == NULL || engine->window == NULL)
		return;

	if (engine->fpsLimit > 0) {
		const double targetFrameTime = 1.0 / (double)engine->fpsLimit;
		double remainingTime = targetFrameTime - (glfwGetTime() - engine->frameStartTime);
		while (remainingTime > 0.0) {
			struct timespec sleepTime = {
				.tv_sec = (time_t)remainingTime,
				.tv_nsec = (long)((remainingTime - (double)(time_t)remainingTime) * 1000000000.0),
			};
			nanosleep(&sleepTime, NULL);
			remainingTime = targetFrameTime - (glfwGetTime() - engine->frameStartTime);
		}
	}

	engine->fpsFrameCount++;
	const double now = glfwGetTime();
	const double fpsWindowDuration = now - engine->fpsWindowStartTime;
	if (fpsWindowDuration >= 1.0) {
		const double fps = (double)engine->fpsFrameCount / fpsWindowDuration;
		char windowTitle[sizeof(engine->title) + 64];
		snprintf(windowTitle, sizeof(windowTitle), "%s - FPS: %.1f%s", engine->title, fps,
		         engine->fpsLimit == 0 ? " (uncapped)" : "");
		glfwSetWindowTitle(engine->window, windowTitle);
		printf("\rFPS: %.1f%s", fps, engine->fpsLimit == 0 ? " (uncapped)" : "");
		fflush(stdout);
		engine->fpsFrameCount = 0;
		engine->fpsWindowStartTime = now;
	}
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
