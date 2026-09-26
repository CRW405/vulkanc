#include "vEngine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Validation layers
// Vulkan does not include validation by default. Validation layers add helpful
// error checking during development without changing the normal API flow.
//
// Ask Vulkan, "Can you watch my code and point out mistakes?"
// -----------------------------------------------------------------------------

static const bool enableValidationLayers = true;
static const char *validationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};

static bool checkValidationLayerSupport(void) {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
	if (availableLayers == NULL) {
		return false;
	}
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	const uint32_t validationLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
	for (uint32_t i = 0; i < validationLayerCount; i++) {
		bool layerFound = false;
		for (uint32_t j = 0; j < layerCount; j++) {
			if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			free(availableLayers);
			return false;
		}
	}

	free(availableLayers);
	return true;
}

// -----------------------------------------------------------------------------
// Physical-device and queue-family helpers
// A Queue Family is a group of queues that support a specific set of operations.
// For example, a queue family may support graphics operations, while another may
// support compute operations. A physical device may have multiple queue families.
//
// Look at each GPU and find one that can draw pictures and show pictures.
// -----------------------------------------------------------------------------

typedef struct QueueFamilyIndices {
	uint32_t graphicsFamily;
	bool hasGraphicsFamily;
	uint32_t presentFamily;
	bool hasPresentFamily;
} QueueFamilyIndices;

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices = { 0 };
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

	VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	if (queueFamilies == NULL) {
		return indices;
	}
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
			indices.hasGraphicsFamily = true;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
			indices.hasPresentFamily = true;
		}

		if (indices.hasGraphicsFamily && indices.hasPresentFamily) {
			break;
		}
	}

	free(queueFamilies);
	return indices;
}

static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices = findQueueFamilies(device, surface);
	return indices.hasGraphicsFamily && indices.hasPresentFamily;
}

// -----------------------------------------------------------------------------
// Swapchain helpers
// A swapchain is a queue of images that are presented to the screen. The swapchain
// is created with a specific format, color space, and presentation mode.
//
// Ask the window what kind of pictures it accepts, then choose settings that fit.
// -----------------------------------------------------------------------------

typedef struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	uint32_t formatCount;
	VkSurfaceFormatKHR *formats;
	uint32_t presentModeCount;
	VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

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

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities) {
	if (capabilities->currentExtent.width != UINT32_MAX) {
		return capabilities->currentExtent;
	}

	VkExtent2D actualExtent = { VENGINE_WINDOW_WIDTH, VENGINE_WINDOW_HEIGHT };
	if (actualExtent.width < capabilities->minImageExtent.width) actualExtent.width = capabilities->minImageExtent.width;
	if (actualExtent.width > capabilities->maxImageExtent.width) actualExtent.width = capabilities->maxImageExtent.width;
	if (actualExtent.height < capabilities->minImageExtent.height) actualExtent.height = capabilities->minImageExtent.height;
	if (actualExtent.height > capabilities->maxImageExtent.height) actualExtent.height = capabilities->maxImageExtent.height;
	return actualExtent;
}

static bool createInstance(vEngine *engine) {
	// -----------------------------------------------------------------------------
	// Vulkan instance setup
	// The instance is the connection between the application and the Vulkan library.
	//
	// Introduce our program to Vulkan, turn on the tools we need, and connect Vulkan
	// to the window so Vulkan has a place to put pictures.
	// -----------------------------------------------------------------------------
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		fprintf(stderr, "Validation layers requested, but not available!\n");
		return false;
	}

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello World",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "vEngine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	if (glfwExtensions == NULL) {
		fprintf(stderr, "Failed to find GLFW Vulkan extensions\n");
		return false;
	}

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions,
	};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;
	}

	if (vkCreateInstance(&createInfo, NULL, &engine->instance) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan instance\n");
		return false;
	}
	return true;
}

static bool selectPhysicalDevice(vEngine *engine) {
	// Pick the first GPU that provides both graphics and presentation queues.
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(engine->instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
		return false;
	}

	VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
	if (devices == NULL) return false;
	vkEnumeratePhysicalDevices(engine->instance, &deviceCount, devices);
	for (uint32_t i = 0; i < deviceCount; i++) {
		if (isDeviceSuitable(devices[i], engine->surface)) {
			engine->physicalDevice = devices[i];
			break;
		}
	}
	free(devices);
	if (engine->physicalDevice == VK_NULL_HANDLE) {
		fprintf(stderr, "Failed to find a suitable GPU\n");
		return false;
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(engine->physicalDevice, &properties);
	printf("GPU selected: %s\n", properties.deviceName);
	return true;
}

static bool createLogicalDevice(vEngine *engine, QueueFamilyIndices indices) {
	// -----------------------------------------------------------------------------
	// Logical device and queue setup
	// A logical device is a handle to the physical device that allows us to interact with it.
	// A queue is a command buffer that can be submitted to the GPU for execution.
	//
	// Choose the GPU's worker, give it a work line, and get that work line back so
	// the program can send drawing jobs to the GPU.
	// -----------------------------------------------------------------------------
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfos[2] = { {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = indices.graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	} };
	uint32_t queueCreateInfoCount = 1;
	if (indices.graphicsFamily != indices.presentFamily) {
		queueCreateInfos[1] = (VkDeviceQueueCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = indices.presentFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		};
		queueCreateInfoCount = 2;
	}
	const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo deviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = queueCreateInfoCount,
		.pQueueCreateInfos = queueCreateInfos,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = deviceExtensions,
	};
	if (vkCreateDevice(engine->physicalDevice, &deviceCreateInfo, NULL, &engine->device) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create logical device\n");
		return false;
	}
	vkGetDeviceQueue(engine->device, indices.graphicsFamily, 0, &engine->graphicsQueue);
	vkGetDeviceQueue(engine->device, indices.presentFamily, 0, &engine->presentQueue);
	printf("Logical device and graphics queue created successfully\n");
	return true;
}

static bool createSwapchain(vEngine *engine, QueueFamilyIndices indices) {
	// -----------------------------------------------------------------------------
	// Swapchain and image-view setup
	// A swapchain is a queue of images that are presented to the screen. The swapchain
	// is created with a specific format, color space, and presentation mode.
	// An image view is a handle to an image that allows us to access its pixels.
	//
	// Make a small line of pictures. While the screen shows one picture, Vulkan can
	// prepare another. Make a view for every picture so Vulkan knows how to use it.
	// -----------------------------------------------------------------------------
	SwapChainSupportDetails support = querySwapChainSupport(engine->physicalDevice, engine->surface);
	if (support.formats == NULL || support.presentModes == NULL) {
		free(support.formats);
		free(support.presentModes);
		fprintf(stderr, "Failed to query swapchain support\n");
		return false;
	}
	VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(support.formats, support.formatCount);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes, support.presentModeCount);
	engine->swapchainExtent = chooseSwapExtent(&support.capabilities);
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
		free(support.formats); free(support.presentModes);
		return false;
	}
	vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &engine->swapchainImageCount, NULL);
	engine->swapchainImages = calloc(engine->swapchainImageCount, sizeof(VkImage));
	engine->swapchainImageViews = calloc(engine->swapchainImageCount, sizeof(VkImageView));
	engine->swapchainFramebuffers = calloc(engine->swapchainImageCount, sizeof(VkFramebuffer));
	if (engine->swapchainImages == NULL || engine->swapchainImageViews == NULL || engine->swapchainFramebuffers == NULL) {
		free(support.formats); free(support.presentModes);
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
			free(support.formats); free(support.presentModes);
			return false;
		}
	}
	free(support.formats); free(support.presentModes);
	printf("Image views created successfully\n");
	return true;
}

static bool createRenderPassAndFramebuffers(vEngine *engine) {
	// -----------------------------------------------------------------------------
	// Render pass and framebuffer setup
	// A render pass describes the attachments and operations used for one draw.
	// A framebuffer is a collection of attachments that are used as the destination for rendering.
	//
	// Tell Vulkan, "For each picture, start by clearing it, let drawing happen,
	// then leave the finished color ready for the screen." Build one framebuffer
	// for each swapchain picture.
	// -----------------------------------------------------------------------------
	VkAttachmentDescription colorAttachment = {
		.format = engine->swapchainImageFormat, .samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = 1, .pColorAttachments = &colorReference };
	VkRenderPassCreateInfo renderPassInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .attachmentCount = 1, .pAttachments = &colorAttachment, .subpassCount = 1, .pSubpasses = &subpass };
	if (vkCreateRenderPass(engine->device, &renderPassInfo, NULL, &engine->renderPass) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create render pass\n");
		return false;
	}
	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		VkImageView attachments[] = { engine->swapchainImageViews[i] };
		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .renderPass = engine->renderPass,
			.attachmentCount = 1, .pAttachments = attachments, .width = engine->swapchainExtent.width,
			.height = engine->swapchainExtent.height, .layers = 1,
		};
		if (vkCreateFramebuffer(engine->device, &framebufferInfo, NULL, &engine->swapchainFramebuffers[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create framebuffer %u\n", i);
			return false;
		}
	}
	return true;
}

static bool createCommandResources(vEngine *engine, uint32_t graphicsFamily) {
	// -----------------------------------------------------------------------------
	// Command buffer and synchronization setup
	// A command buffer is a sequence of commands that will be submitted to the GPU for execution.
	//
	// Make a reusable instruction list and a few traffic lights. The traffic lights
	// stop the GPU from using a picture before it is ready or showing it too early.
	// -----------------------------------------------------------------------------
	VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = graphicsFamily };
	if (vkCreateCommandPool(engine->device, &poolInfo, NULL, &engine->commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		return false;
	}
	VkCommandBufferAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = engine->commandPool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
	if (vkAllocateCommandBuffers(engine->device, &allocInfo, &engine->commandBuffer) != VK_SUCCESS) return false;

	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
	if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL, &engine->imageAvailableSemaphore) != VK_SUCCESS ||
	    vkCreateFence(engine->device, &fenceInfo, NULL, &engine->inFlightFence) != VK_SUCCESS) return false;
	engine->renderFinishedSemaphores = calloc(engine->swapchainImageCount, sizeof(VkSemaphore));
	if (engine->renderFinishedSemaphores == NULL) return false;
	for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
		if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL, &engine->renderFinishedSemaphores[i]) != VK_SUCCESS) return false;
	}
	return true;
}

bool vEngineCreate(vEngine *engine, GLFWwindow *window) {
	if (engine == NULL || window == NULL || !createInstance(engine)) return false;
	if (glfwCreateWindowSurface(engine->instance, window, NULL, &engine->surface) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create window surface\n");
		return false;
	}
	if (!selectPhysicalDevice(engine)) return false;
	QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);
	return createLogicalDevice(engine, indices) &&
	       createSwapchain(engine, indices) &&
	       createRenderPassAndFramebuffers(engine) &&
	       createCommandResources(engine, indices.graphicsFamily);
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
	vkWaitForFences(engine->device, 1, &engine->inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(engine->device, 1, &engine->inFlightFence);
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(engine->device, engine->swapchain, UINT64_MAX, engine->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS) return;

	vkResetCommandBuffer(engine->commandBuffer, 0);
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(engine->commandBuffer, &beginInfo);
	VkClearValue clearColor = { { { 0.5f, 0.0f, 0.5f, 1.0f } } }; // rgba
	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .renderPass = engine->renderPass,
		.framebuffer = engine->swapchainFramebuffers[imageIndex], .renderArea.extent = engine->swapchainExtent,
		.clearValueCount = 1, .pClearValues = &clearColor,
	};
	vkCmdBeginRenderPass(engine->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(engine->commandBuffer);
	vkEndCommandBuffer(engine->commandBuffer);

	VkSemaphore waitSemaphores[] = { engine->imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { engine->renderFinishedSemaphores[imageIndex] };
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .waitSemaphoreCount = 1, .pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages, .commandBufferCount = 1, .pCommandBuffers = &engine->commandBuffer,
		.signalSemaphoreCount = 1, .pSignalSemaphores = signalSemaphores,
	};
	vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, engine->inFlightFence);
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, .waitSemaphoreCount = 1, .pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1, .pSwapchains = &engine->swapchain, .pImageIndices = &imageIndex,
	};
	vkQueuePresentKHR(engine->presentQueue, &presentInfo);
}

// -----------------------------------------------------------------------------
// Cleanup
//
// Wait for the GPU to finish, then destroy the things we created in reverse
// order so nothing is still using something we already removed.
// -----------------------------------------------------------------------------

void vEngineDestroy(vEngine *engine) {
	if (engine == NULL) return;
	if (engine->device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(engine->device);
		for (uint32_t i = 0; i < engine->swapchainImageCount; i++) {
			if (engine->swapchainFramebuffers != NULL) vkDestroyFramebuffer(engine->device, engine->swapchainFramebuffers[i], NULL);
			if (engine->swapchainImageViews != NULL) vkDestroyImageView(engine->device, engine->swapchainImageViews[i], NULL);
			if (engine->renderFinishedSemaphores != NULL) vkDestroySemaphore(engine->device, engine->renderFinishedSemaphores[i], NULL);
		}
		if (engine->renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(engine->device, engine->renderPass, NULL);
		if (engine->commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(engine->device, engine->commandPool, NULL);
		if (engine->imageAvailableSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(engine->device, engine->imageAvailableSemaphore, NULL);
		if (engine->inFlightFence != VK_NULL_HANDLE) vkDestroyFence(engine->device, engine->inFlightFence, NULL);
		if (engine->swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(engine->device, engine->swapchain, NULL);
		vkDestroyDevice(engine->device, NULL);
	}
	if (engine->surface != VK_NULL_HANDLE && engine->instance != VK_NULL_HANDLE) vkDestroySurfaceKHR(engine->instance, engine->surface, NULL);
	if (engine->instance != VK_NULL_HANDLE) vkDestroyInstance(engine->instance, NULL);
	free(engine->swapchainImages);
	free(engine->swapchainImageViews);
	free(engine->swapchainFramebuffers);
	free(engine->renderFinishedSemaphores);
	*engine = (vEngine){ 0 };
}
