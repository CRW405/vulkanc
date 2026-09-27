#include "glfwEngine.h"
#include "vEngine.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
	glfwEngine glfw = { 0 };
	if (!glfwEngineCreate(&glfw)) {
		glfwEngineDestroy(&glfw);
		return EXIT_FAILURE;
	}

	vEngine engine = { 0 };
	if (!vEngineCreate(&engine, glfw.window)) {
		fprintf(stderr, "Failed to create Vulkan engine\n");
		vEngineDestroy(&engine);
		glfwEngineDestroy(&glfw);
		return EXIT_FAILURE;
	}

	// main loop
	while (!glfwWindowShouldClose(glfw.window)) {
		glfwPollEvents();

		if (glfwGetKey(glfw.window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
		    glfwGetKey(glfw.window, GLFW_KEY_Q) == GLFW_PRESS) {
			glfwSetWindowShouldClose(glfw.window, GLFW_TRUE);
		}

		vEngineDrawFrame(&engine);
	}

	vEngineDestroy(&engine);
	glfwEngineDestroy(&glfw);

	printf("Cleanup complete. Exiting program.\n");
	return EXIT_SUCCESS;
}
