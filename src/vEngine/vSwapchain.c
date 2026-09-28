#include "vEngine.h"

// -----------------------------------------------------------------------------
// Swapchain helpers
// A swapchain is a queue of images that are presented to the screen. The swapchain
// is created with a specific format, color space, and presentation mode.
//
// Ask the window what kind of pictures it accepts, then choose settings that fit.
// -----------------------------------------------------------------------------

static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportDetails details = { 0 };

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, NULL);
	if (details.formatCount != 0) {
		details.formats = malloc(sizeof(VkSurfaceFormatKHR) * details.formatCount);
		if (details.formats != NULL) {
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, details.formats);
		}
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, NULL);
	if (details.presentModeCount != 0) {
		details.presentModes = malloc(sizeof(VkPresentModeKHR) * details.presentModeCount);
		if (details.presentModes != NULL) {
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, details.presentModes);
		}
	}
	return details;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats, uint32_t formatCount) {
	for (uint32_t i = 0; i < formatCount; i++) {
		if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
		    availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormats[i];
		}
	}
	return availableFormats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount) {
	for (uint32_t i = 0; i < presentModeCount; i++) {
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentModes[i];
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities, GLFWwindow *window) {
	if (capabilities->currentExtent.width != UINT32_MAX) {
		return capabilities->currentExtent;
	}

	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
	if (actualExtent.width < capabilities->minImageExtent.width)
		actualExtent.width = capabilities->minImageExtent.width;
	if (actualExtent.width > capabilities->maxImageExtent.width)
		actualExtent.width = capabilities->maxImageExtent.width;
	if (actualExtent.height < capabilities->minImageExtent.height)
		actualExtent.height = capabilities->minImageExtent.height;
	if (actualExtent.height > capabilities->maxImageExtent.height)
		actualExtent.height = capabilities->maxImageExtent.height;
	return actualExtent;
}

// -----------------------------------------------------------------------------
// Swapchain and image-view setup
// A swapchain is a queue of images that are presented to the screen. The swapchain
// is created with a specific format, color space, and presentation mode.
// An image view is a handle to an image that allows us to access its pixels.
//
// Make a small line of pictures. While the screen shows one picture, Vulkan can
// prepare another. Make a view for every picture so Vulkan knows how to use it.
// -----------------------------------------------------------------------------
bool vEngineCreateSwapchain(vEngine *engine, QueueFamilyIndices indices) {
	if (engine == NULL || engine->window == NULL)
		return false;

	SwapChainSupportDetails support = querySwapChainSupport(engine->physicalDevice, engine->surface);
	if (support.formats == NULL || support.presentModes == NULL) {
		free(support.formats);
		free(support.presentModes);
		fprintf(stderr, "Failed to query swapchain support\n");
		return false;
	}
	VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(support.formats, support.formatCount);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes, support.presentModeCount);
	engine->swapchainExtent = chooseSwapExtent(&support.capabilities, engine->window);
	engine->swapchainImageCount = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 && engine->swapchainImageCount > support.capabilities.maxImageCount)
		engine->swapchainImageCount = support.capabilities.maxImageCount;

	uint32_t familyIndices[] = { indices.graphicsFamily, indices.presentFamily };
	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = engine->surface,
		.minImageCount = engine->swapchainImageCount,
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
		.imageExtent = engine->swapchainExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = indices.graphicsFamily != indices.presentFamily ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = indices.graphicsFamily != indices.presentFamily ? 2 : 0,
		.pQueueFamilyIndices = indices.graphicsFamily != indices.presentFamily ? familyIndices : NULL,
		.preTransform = support.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
	};
	if (vkCreateSwapchainKHR(engine->device, &createInfo, NULL, &engine->swapchain) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create swap chain\n");
		free(support.formats);
		free(support.presentModes);
		return false;
	}
	vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &engine->swapchainImageCount, NULL);
	engine->swapchainImages = calloc(engine->swapchainImageCount, sizeof(VkImage));
	engine->swapchainImageViews = calloc(engine->swapchainImageCount, sizeof(VkImageView));
	engine->swapchainFramebuffers = calloc(engine->swapchainImageCount, sizeof(VkFramebuffer));
	if (engine->swapchainImages == NULL || engine->swapchainImageViews == NULL || engine->swapchainFramebuffers == NULL) {
		free(support.formats);
		free(support.presentModes);
		fprintf(stderr, "Failed to allocate swapchain resources\n");
		return false;
	}
	vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &engine->swapchainImageCount, engine->swapchainImages);
	engine->swapchainImageFormat = format.format;
	printf("Swap chain created successfully with %u images\n", engine->swapchainImageCount);

	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		VkImageViewCreateInfo viewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = engine->swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = engine->swapchainImageFormat,
			.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		};
		if (vkCreateImageView(engine->device, &viewInfo, NULL, &engine->swapchainImageViews[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image view %u\n", i);
			free(support.formats);
			free(support.presentModes);
			return false;
		}
	}
	free(support.formats);
	free(support.presentModes);
	printf("Image views created successfully\n");
	return true;
}

bool vEngineCleanupSwapchain(vEngine *engine) {
	if (engine == NULL || engine->device == VK_NULL_HANDLE)
		return false;

	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		if (engine->swapchainFramebuffers != NULL)
			vkDestroyFramebuffer(engine->device, engine->swapchainFramebuffers[i], NULL);
		if (engine->swapchainImageViews != NULL)
			vkDestroyImageView(engine->device, engine->swapchainImageViews[i], NULL);
	}
	free(engine->swapchainImages);
	free(engine->swapchainImageViews);
	free(engine->swapchainFramebuffers);
	engine->swapchainImages = NULL;
	engine->swapchainImageViews = NULL;
	engine->swapchainFramebuffers = NULL;

	if (engine->swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(engine->device, engine->swapchain, NULL);
		engine->swapchain = VK_NULL_HANDLE;
	}
	return true;
}

bool vEngineRecreateSwapchain(vEngine *engine) {
	int width = 0, height = 0;
	glfwGetFramebufferSize(engine->window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(engine->window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(engine->device);

	vEngineDestroyRenderFinishedSemaphores(engine);
	vEngineCleanupSwapchain(engine);

	QueueFamilyIndices indices = vEngineFindQueueFamilies(engine->physicalDevice, engine->surface);
	if (!vEngineCreateSwapchain(engine, indices)) {
		return false;
	}
	if (!vEngineCreateRenderFinishedSemaphores(engine)) {
		fprintf(stderr, "Failed to recreate render-finished semaphores\n");
		return false;
	}

	// Recreate framebuffers to match the new image views and swapchain count
	engine->swapchainFramebuffers = calloc(engine->swapchainImageCount, sizeof(VkFramebuffer));
	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		VkImageView attachments[] = { engine->swapchainImageViews[i] };
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = engine->renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = engine->swapchainExtent.width,
			.height = engine->swapchainExtent.height,
			.layers = 1,
		};
		if (vkCreateFramebuffer(engine->device, &framebufferInfo, NULL, &engine->swapchainFramebuffers[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to recreate framebuffer %u\n", i);
			return false;
		}
	}
	return true;
}
