#include "vEngine.h"

// -----------------------------------------------------------------------------
// Command buffer and synchronization setup
// A command buffer is a sequence of commands that will be submitted to the GPU for execution.
//
// Make a reusable instruction list and a few traffic lights. The traffic lights
// stop the GPU from using a picture before it is ready or showing it too early.
// -----------------------------------------------------------------------------

bool vEngineCreateCommandResources(vEngine *engine, uint32_t graphicsFamily) {
	VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = graphicsFamily };
	if (vkCreateCommandPool(engine->device, &poolInfo, NULL, &engine->commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		return false;
	}
	VkCommandBufferAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = engine->commandPool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
	if (vkAllocateCommandBuffers(engine->device, &allocInfo, &engine->commandBuffer) != VK_SUCCESS)
		return false;

	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
	if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL, &engine->imageAvailableSemaphore) != VK_SUCCESS ||
	    vkCreateFence(engine->device, &fenceInfo, NULL, &engine->inFlightFence) != VK_SUCCESS)
		return false;
	engine->renderFinishedSemaphores = calloc(engine->swapchainImageCount, sizeof(VkSemaphore));
	if (engine->renderFinishedSemaphores == NULL)
		return false;
	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL, &engine->renderFinishedSemaphores[i]) != VK_SUCCESS)
			return false;
	}
	return true;
}
