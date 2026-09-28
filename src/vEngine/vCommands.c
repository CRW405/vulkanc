#include "vEngine.h"

bool vEngineCreateRenderFinishedSemaphores(vEngine *engine) {
	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	engine->renderFinishedSemaphores = calloc(engine->swapchainImageCount, sizeof(VkSemaphore));
	if (engine->renderFinishedSemaphores == NULL)
		return false;

	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL,
				      &engine->renderFinishedSemaphores[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create render-finished semaphore %u\n", i);
			vEngineDestroyRenderFinishedSemaphores(engine);
			return false;
		}
	}
	return true;
}

void vEngineDestroyRenderFinishedSemaphores(vEngine *engine) {
	if (engine == NULL || engine->renderFinishedSemaphores == NULL)
		return;
	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		if (engine->renderFinishedSemaphores[i] != VK_NULL_HANDLE)
			vkDestroySemaphore(engine->device, engine->renderFinishedSemaphores[i], NULL);
	}
	free(engine->renderFinishedSemaphores);
	engine->renderFinishedSemaphores = NULL;
}

// -----------------------------------------------------------------------------
// Command buffer and synchronization setup
// A command buffer is a sequence of commands that will be submitted to the GPU for execution.
//
// Make a reusable instruction list and a few traffic lights. The traffic lights
// stop the GPU from using a picture before it is ready or showing it too early.
// -----------------------------------------------------------------------------

bool vEngineCreateCommandResources(vEngine *engine, uint32_t graphicsFamily) {
	bool commandBuffersAllocated = false;
	VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = graphicsFamily };
	if (vkCreateCommandPool(engine->device, &poolInfo, NULL, &engine->commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		return false;
	}
	engine->commandBuffers = calloc(MAX_FRAMES_IN_FLIGHT, sizeof(VkCommandBuffer));
	if (engine->commandBuffers == NULL)
		goto fail;
	VkCommandBufferAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = engine->commandPool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = MAX_FRAMES_IN_FLIGHT };
	commandBuffersAllocated = vkAllocateCommandBuffers(engine->device, &allocInfo, engine->commandBuffers) == VK_SUCCESS;
	if (!commandBuffersAllocated) {
		fprintf(stderr, "Failed to allocate command buffers\n");
		goto fail;
	}

	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

	engine->imageAvailableSemaphores = calloc(MAX_FRAMES_IN_FLIGHT, sizeof(VkSemaphore));
	engine->inFlightFences = calloc(MAX_FRAMES_IN_FLIGHT, sizeof(VkFence));
	engine->imagesInFlight = calloc(engine->swapchainImageCount, sizeof(VkFence));
	if (engine->imageAvailableSemaphores == NULL ||
	    engine->inFlightFences == NULL || engine->imagesInFlight == NULL)
		goto fail;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL, &engine->imageAvailableSemaphores[i]) != VK_SUCCESS ||
		    vkCreateFence(engine->device, &fenceInfo, NULL, &engine->inFlightFences[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create frame synchronization objects\n");
			goto fail;
		}
	}
	if (!vEngineCreateRenderFinishedSemaphores(engine))
		goto fail;
	engine->currentFrame = 0;
	return true;

fail:
	vEngineDestroyRenderFinishedSemaphores(engine);
	if (engine->device != VK_NULL_HANDLE) {
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (engine->imageAvailableSemaphores != NULL &&
			    engine->imageAvailableSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(engine->device, engine->imageAvailableSemaphores[i], NULL);
			if (engine->inFlightFences != NULL && engine->inFlightFences[i] != VK_NULL_HANDLE)
				vkDestroyFence(engine->device, engine->inFlightFences[i], NULL);
		}
		if (commandBuffersAllocated)
			vkFreeCommandBuffers(engine->device, engine->commandPool,
					     MAX_FRAMES_IN_FLIGHT, engine->commandBuffers);
		if (engine->commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(engine->device, engine->commandPool, NULL);
	}
	free(engine->imageAvailableSemaphores);
	free(engine->inFlightFences);
	free(engine->imagesInFlight);
	free(engine->commandBuffers);
	engine->imageAvailableSemaphores = NULL;
	engine->inFlightFences = NULL;
	engine->imagesInFlight = NULL;
	engine->commandBuffers = NULL;
	engine->commandPool = VK_NULL_HANDLE;
	return false;
}
