#include "vEngine.h"

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
	(void)width;
	(void)height;
	vEngine *engine = (vEngine *)glfwGetWindowUserPointer(window);
	if (engine != NULL)
		engine->framebufferResized = true;
}

bool vEngineCreate(vEngine *engine, GLFWwindow *window) {
	if (engine == NULL || window == NULL)
		return false;
	engine->window = window;
	engine->framebufferResized = false;
	glfwSetWindowUserPointer(window, engine);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	if (!vEngineCreateInstance(engine))
		return false;
	vEngineSetClearColor(engine, 0.0f, 0.0f, 0.0f, 1.0f);
	if (glfwCreateWindowSurface(engine->instance, engine->window, NULL, &engine->surface) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create window surface\n");
		return false;
	}
	if (!vEngineSelectPhysicalDevice(engine))
		return false;
	QueueFamilyIndices indices = vEngineFindQueueFamilies(engine->physicalDevice, engine->surface);
	return vEngineCreateLogicalDevice(engine, indices) &&
	       vEngineCreateSwapchain(engine, indices) &&
	       vEngineCreateRenderPassAndFramebuffers(engine) &&
	       vEngineCreateCommandResources(engine, indices.graphicsFamily);
}

bool vEngineLoadShaders(vEngine *engine, const char *vertexShaderPath, const char *fragmentShaderPath) {
	if (engine == NULL || engine->device == VK_NULL_HANDLE ||
	    vertexShaderPath == NULL || fragmentShaderPath == NULL)
		return false;

	vkDeviceWaitIdle(engine->device);
	if (engine->graphicsPipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(engine->device, engine->graphicsPipeline, NULL);
	if (engine->pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(engine->device, engine->pipelineLayout, NULL);
	engine->graphicsPipeline = VK_NULL_HANDLE;
	engine->pipelineLayout = VK_NULL_HANDLE;

	return vEngineCreateGraphicsPipeline(engine, vertexShaderPath, fragmentShaderPath);
}

void vEngineSetClearColor(vEngine *engine, float red, float green, float blue, float alpha) {
	if (engine == NULL)
		return;
	engine->clearColor.color.float32[0] = red;
	engine->clearColor.color.float32[1] = green;
	engine->clearColor.color.float32[2] = blue;
	engine->clearColor.color.float32[3] = alpha;
}

// -----------------------------------------------------------------------------
// Main render loop
//
// Repeat this simple picture-making job:
// 1. Get a free swapchain picture.
// 2. Write down commands that clear it purple.
// 3. Send those commands to the GPU.
// 4. Wait until the GPU is done.
// 5. Show that picture in the window.
// -----------------------------------------------------------------------------

void vEngineDrawFrame(vEngine *engine) {
	uint32_t frame = engine->currentFrame;
	VkFence currentFrameFence = engine->inFlightFences[frame];
	vkWaitForFences(engine->device, 1, &currentFrameFence, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
	    engine->device,
	    engine->swapchain,
	    UINT64_MAX,
	    engine->imageAvailableSemaphores[frame],
	    VK_NULL_HANDLE,
	    &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		vEngineRecreateSwapchain(engine);
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		fprintf(stderr, "Failed to acquire swap chain image!\n");
		return;
	}

	if (engine->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(engine->device, 1, &engine->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	engine->imagesInFlight[imageIndex] = currentFrameFence;

	vkResetFences(engine->device, 1, &currentFrameFence);

	vkResetCommandBuffer(engine->commandBuffers[frame], 0);
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(engine->commandBuffers[frame], &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = engine->renderPass,
		.framebuffer = engine->swapchainFramebuffers[imageIndex],
		.renderArea.extent = engine->swapchainExtent,
		.clearValueCount = 1,
		.pClearValues = &engine->clearColor,
	};
	vkCmdBeginRenderPass(engine->commandBuffers[frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	if (engine->graphicsPipeline != VK_NULL_HANDLE &&
	    engine->vertexBuffer != VK_NULL_HANDLE && engine->vertexCount > 0) {
		vkCmdBindPipeline(engine->commandBuffers[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, engine->graphicsPipeline);

		VkBuffer vertexBuffers[] = { engine->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(engine->commandBuffers[frame], 0, 1, vertexBuffers, offsets);
		vkCmdDraw(engine->commandBuffers[frame], engine->vertexCount, 1, 0, 0);
	}

	vkCmdEndRenderPass(engine->commandBuffers[frame]);
	vkEndCommandBuffer(engine->commandBuffers[frame]);

	VkSemaphore waitSemaphores[] = { engine->imageAvailableSemaphores[frame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { engine->renderFinishedSemaphores[imageIndex] };

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &engine->commandBuffers[frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};

	vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, currentFrameFence);

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = &engine->swapchain,
		.pImageIndices = &imageIndex,
	};

	result = vkQueuePresentKHR(engine->presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || engine->framebufferResized) {
		engine->framebufferResized = false;
		vEngineRecreateSwapchain(engine);
	} else if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to present swap chain image!\n");
	}

	engine->currentFrame = (engine->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// -----------------------------------------------------------------------------
// Cleanup
//
// Wait for the GPU to finish, then destroy the things we created in reverse
// order so nothing is still using something we already removed.
// -----------------------------------------------------------------------------

void vEngineDestroy(vEngine *engine) {
	if (engine == NULL)
		return;
	if (engine->device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(engine->device);
		if (engine->vertexBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(engine->device, engine->vertexBuffer, NULL);
		if (engine->vertexBufferMemory != VK_NULL_HANDLE)
			vkFreeMemory(engine->device, engine->vertexBufferMemory, NULL);
		if (engine->graphicsPipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(engine->device, engine->graphicsPipeline, NULL);
		if (engine->pipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(engine->device, engine->pipelineLayout, NULL);
		for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
			if (engine->swapchainFramebuffers != NULL)
				vkDestroyFramebuffer(engine->device, engine->swapchainFramebuffers[i], NULL);
			if (engine->swapchainImageViews != NULL)
				vkDestroyImageView(engine->device, engine->swapchainImageViews[i], NULL);
		}
		vEngineDestroyRenderFinishedSemaphores(engine);
		if (engine->renderPass != VK_NULL_HANDLE)
			vkDestroyRenderPass(engine->device, engine->renderPass, NULL);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (engine->imageAvailableSemaphores != NULL &&
			    engine->imageAvailableSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(engine->device, engine->imageAvailableSemaphores[i], NULL);
		}
		if (engine->commandBuffers != NULL)
			vkFreeCommandBuffers(engine->device, engine->commandPool,
					     MAX_FRAMES_IN_FLIGHT, engine->commandBuffers);
		if (engine->commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(engine->device, engine->commandPool, NULL);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (engine->inFlightFences != NULL && engine->inFlightFences[i] != VK_NULL_HANDLE)
				vkDestroyFence(engine->device, engine->inFlightFences[i], NULL);
		}
		if (engine->swapchain != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(engine->device, engine->swapchain, NULL);
		vkDestroyDevice(engine->device, NULL);
	}
	if (engine->surface != VK_NULL_HANDLE && engine->instance != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(engine->instance, engine->surface, NULL);
	if (engine->instance != VK_NULL_HANDLE)
		vkDestroyInstance(engine->instance, NULL);
	free(engine->swapchainImages);
	free(engine->swapchainImageViews);
	free(engine->swapchainFramebuffers);
	free(engine->renderFinishedSemaphores);
	free(engine->imageAvailableSemaphores);
	free(engine->commandBuffers);
	free(engine->inFlightFences);
	free(engine->imagesInFlight);
	*engine = (vEngine){ 0 };
}
