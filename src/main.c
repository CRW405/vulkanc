#include "glfwEngine.h"
#include "vEngine.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
	const glfwEngineConfig windowConfig = {
		.width = 500,
		.height = 500,
		.title = "Hello World",
		.fpsLimit = 0,
	};
	glfwEngine glfw = { 0 };
	if (!glfwEngineCreate(&glfw, &windowConfig)) {
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

	float triangle1Size = 1.0f;
	const Vertex triangleVertices[] = {
		{ .position = { 0.0f, -triangle1Size, 0.0f }, .color = { 1.0f, 0.0f, 0.0f } },
		{ .position = { 0.5f, triangle1Size, 0.0f },  .color = { 0.0f, 1.0f, 0.0f } },
		{ .position = { -triangle1Size, 0.5f, 0.0f }, .color = { 0.0f, 0.0f, 1.0f } },
	};
	if (!vEngineCreateVertexBuffer(&engine, triangleVertices, 3)) {
		fprintf(stderr, "Failed to create triangle vertex buffer\n");
		vEngineDestroy(&engine);
		glfwEngineDestroy(&glfw);
		return EXIT_FAILURE;
	}

	vEngineSetClearColor(&engine, 0.5f, 0.0f, 0.5f, 1.0f);
	// vEngineLoadShaders(&engine, "./shaders/static_triangle.vert.spv", "./shaders/triangle.frag.spv");
	vEngineLoadShaders(&engine, "./shaders/triangle.vert.spv", "./shaders/triangle.frag.spv");

	// main loop
	while (!glfwWindowShouldClose(glfw.window)) {
		glfwEngineFrameStart(&glfw);
		glfwPollEvents();

		if (glfwGetKey(glfw.window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
		    glfwGetKey(glfw.window, GLFW_KEY_Q) == GLFW_PRESS) {
			glfwSetWindowShouldClose(glfw.window, GLFW_TRUE);
		}

		vEngineDrawFrame(&engine);
		glfwEngineFrameEnd(&glfw);
	}

	vEngineDestroy(&engine);
	glfwEngineDestroy(&glfw);

	printf("\n");
	printf("Cleanup complete. Exiting program.\n");
	return EXIT_SUCCESS;
}
