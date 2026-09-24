#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500

// -----------------------------------------------------------------------------
// Validation layers
// Vulkan does not include validation by default. Validation layers add helpful
// error checking during development without changing the normal API flow.
//
// Ask Vulkan, "Can you watch my code and point out mistakes?"
// -----------------------------------------------------------------------------

const bool  enableValidationLayers = true;
const char *validationLayers[]     = {
	"VK_LAYER_KHRONOS_validation"
};

bool checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
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
	bool     hasGraphicsFamily;
	uint32_t presentFamily;
	bool     hasPresentFamily;
} QueueFamilyIndices;

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices          = { 0 };
	uint32_t           queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

	VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily    = i;
			indices.hasGraphicsFamily = true;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily    = i;
			indices.hasPresentFamily = true;
		}

		if (indices.hasGraphicsFamily && indices.hasPresentFamily) {
			break;
		}
	}

	free(queueFamilies);
	return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
	uint32_t                 formatCount;
	VkSurfaceFormatKHR      *formats;
	uint32_t                 presentModeCount;
	VkPresentModeKHR        *presentModes;
} SwapChainSupportDetails;

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportDetails details = { 0 };

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, NULL);
	if (details.formatCount != 0) {
		details.formats = malloc(sizeof(VkSurfaceFormatKHR) * details.formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, details.formats);
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, NULL);
	if (details.presentModeCount != 0) {
		details.presentModes = malloc(sizeof(VkPresentModeKHR) * details.presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, details.presentModes);
	}
	return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats, uint32_t formatCount) {
	for (uint32_t i = 0; i < formatCount; i++) {
		if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormats[i];
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount) {
	for (uint32_t i = 0; i < presentModeCount; i++) {
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentModes[i];
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities) {
	if (capabilities->currentExtent.width != UINT32_MAX) {
		return capabilities->currentExtent;
	} else {
		VkExtent2D actualExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
		actualExtent.width      = (actualExtent.width < capabilities->minImageExtent.width) ? capabilities->minImageExtent.width : actualExtent.width;
		actualExtent.width      = (actualExtent.width > capabilities->maxImageExtent.width) ? capabilities->maxImageExtent.width : actualExtent.width;
		actualExtent.height     = (actualExtent.height < capabilities->minImageExtent.height) ? capabilities->minImageExtent.height : actualExtent.height;
		actualExtent.height     = (actualExtent.height > capabilities->maxImageExtent.height) ? capabilities->maxImageExtent.height : actualExtent.height;
		return actualExtent;
	}
}

// -----------------------------------------------------------------------------
// The program
// -----------------------------------------------------------------------------

int main() {

	// -----------------------------------------------------------------------------
	// GLFW window setup
	// Graphics Library FrameWork allows us to create windows, handle inputs and
	// events, and create OpenGL and Vulkan contexts.
	//
	// Make an empty window. Tell GLFW, "Do not use OpenGL. Vulkan will draw here."
	// -----------------------------------------------------------------------------

	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return EXIT_FAILURE;
	}

	// these hints tell GLFW how to behave
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);               // Default is OpenGL, but we want Vulkan
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);                 // We have to hanfle resizing ourselves for vulkan
	glfwWindowHintString(GLFW_WAYLAND_APP_ID, "make-me-float"); // just a little hyprland window rule helper so that the window doesnt tile

	GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello World", NULL, NULL);
	if (!window) {
		fprintf(stderr, "Failed to create GLFW window\n");
		return EXIT_FAILURE;
	}

	// -----------------------------------------------------------------------------
	// Vulkan instance setup
	// The instance is the connection between the application and the Vulkan library.
	//
	// Introduce our program to Vulkan, turn on the tools we need, and connect Vulkan
	// to the window so Vulkan has a place to put pictures.
	// -----------------------------------------------------------------------------

	if (enableValidationLayers && !checkValidationLayerSupport()) {
		fprintf(stderr, "Validation layers requested, but not available!\n");
		return EXIT_FAILURE;
	}

	// Describe the application to Vulkan and request the extensions GLFW needs
	// to connect Vulkan to the window system.
	VkApplicationInfo appInfo = {
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName   = "Hello World",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName        = "No Engine",
		.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion         = VK_API_VERSION_1_0,
	};

	// Get the extensions GLFW needs to connect Vulkan to the window system.
	uint32_t             glfwExtensionCount = 0;
	const char         **glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	VkInstanceCreateInfo instanceCreateInfo = {
		.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo        = &appInfo,
		.enabledExtensionCount   = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions,
	};
	if (enableValidationLayers) {
		instanceCreateInfo.enabledLayerCount   = sizeof(validationLayers) / sizeof(validationLayers[0]);
		instanceCreateInfo.ppEnabledLayerNames = validationLayers;
	}

	VkInstance instance;
	if (vkCreateInstance(&instanceCreateInfo, NULL, &instance) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan instance\n");
		return EXIT_FAILURE;
	}

	// The surface is the platform-specific connection between GLFW and Vulkan.
	VkSurfaceKHR surface; // Think of this as a canvas that vulkan draws to and glfw presents
	if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create window surface\n");
		return EXIT_FAILURE;
	}

	// Pick the first GPU that provides both graphics and presentation queues.
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
	if (physicalDeviceCount == 0) {
		fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
		return EXIT_FAILURE;
	}
	VkPhysicalDevice *physicalDevices = malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		if (isDeviceSuitable(physicalDevices[i], surface)) {
			physicalDevice = physicalDevices[i];
			break;
		}
	}
	free(physicalDevices);
	if (physicalDevice == VK_NULL_HANDLE) {
		fprintf(stderr, "Failed to find a suitable GPU\n");
		return EXIT_FAILURE;
	}
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	printf("GPU selected: %s\n", physicalDeviceProperties.deviceName);

	// -----------------------------------------------------------------------------
	// Logical device and queue setup
	// A logical device is a handle to the physical device that allows us to interact with it.
	// A queue is a command buffer that can be submitted to the GPU for execution.
	//
	// Choose the GPU's worker, give it a work line, and get that work line back so
	// the program can send drawing jobs to the GPU.
	// -----------------------------------------------------------------------------

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	// Create a logical device and retrieve its graphics queue.
	float                   queuePriority   = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = indices.graphicsFamily,
		.queueCount       = 1,
		.pQueuePriorities = &queuePriority,
	};

	// An extension is a feature that is not part of the core Vulkan specification.
	// The swapchain extension is required to present images to the screen.
	const char *deviceExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo deviceCreateInfo = {
		.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount    = 1,
		.pQueueCreateInfos       = &queueCreateInfo,
		.enabledExtensionCount   = 1,
		.ppEnabledExtensionNames = deviceExtensions,
	};

	VkDevice device;
	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create logical device\n");
		return EXIT_FAILURE;
	}

	VkQueue graphicsQueue;
	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	printf("Logical device and graphics queue created successfully\n");

	// -----------------------------------------------------------------------------
	// Swapchain and image-view setup
	// A swapchain is a queue of images that are presented to the screen. The swapchain
	// is created with a specific format, color space, and presentation mode.
	// An image view is a handle to an image that allows us to access its pixels.
	//
	// Make a small line of pictures. While the screen shows one picture, Vulkan can
	// prepare another. Make a view for every picture so Vulkan knows how to use it.
	// -----------------------------------------------------------------------------

	// Query the swapchain support details for the physical device and surface.
	SwapChainSupportDetails swapchainSupport = querySwapChainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR      surfaceFormat    = chooseSwapSurfaceFormat(swapchainSupport.formats, swapchainSupport.formatCount);
	VkPresentModeKHR        presentMode      = chooseSwapPresentMode(swapchainSupport.presentModes, swapchainSupport.presentModeCount);
	VkExtent2D              extent           = chooseSwapExtent(&swapchainSupport.capabilities);

	// Request one more image than the minimum so rendering and presentation can
	// overlap, while respecting the device's maximum.
	uint32_t swapchainImageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0 && swapchainImageCount > swapchainSupport.capabilities.maxImageCount) {
		swapchainImageCount = swapchainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface          = surface,
		.minImageCount    = swapchainImageCount,
		.imageFormat      = surfaceFormat.format,
		.imageColorSpace  = surfaceFormat.colorSpace,
		.imageExtent      = extent,
		.imageArrayLayers = 1,
		.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	};

	// If the graphics and presentation queues are different, we need to specify that the images will be shared between them. Otherwise, we can use exclusive mode which is more efficient.
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };
	if (indices.graphicsFamily != indices.presentFamily) {
		swapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
	} else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	// finish setting up the swapchain create info struct
	swapchainCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode    = presentMode;
	swapchainCreateInfo.clipped        = VK_TRUE;
	swapchainCreateInfo.oldSwapchain   = VK_NULL_HANDLE;

	VkSwapchainKHR swapchain;
	if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, NULL, &swapchain) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create swap chain\n");
		return EXIT_FAILURE;
	}

	// Get the swapchain images.
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
	VkImage *swapchainImages = malloc(sizeof(VkImage) * swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);

	VkFormat swapchainImageFormat = surfaceFormat.format;

	printf("Swap chain created successfully with %u images\n", swapchainImageCount);

	// Create image views for each swapchain image. An image view is a handle to an image that allows us to access its pixels.
	VkImageView *swapchainImageViews = malloc(sizeof(VkImageView) * swapchainImageCount);
	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image                           = swapchainImages[i],
			.viewType                        = VK_IMAGE_VIEW_TYPE_2D,
			.format                          = swapchainImageFormat,
			.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel   = 0,
			.subresourceRange.levelCount     = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount     = 1,
		};

		if (vkCreateImageView(device, &imageViewCreateInfo, NULL, &swapchainImageViews[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image view %u\n", i);
			return EXIT_FAILURE;
		}
	}
	printf("Image views created successfully\n");

	// -----------------------------------------------------------------------------
	// Render pass and framebuffer setup
	// A render pass describes the attachments and operations used for one draw.
	// A framebuffer is a collection of attachments that are used as the destination for rendering.
	//
	// Tell Vulkan, "For each picture, start by clearing it, let drawing happen,
	// then leave the finished color ready for the screen." Build one framebuffer
	// for each swapchain picture.
	// -----------------------------------------------------------------------------

	// The render pass describes the attachments and operations used for one draw. In this case, we have one color attachment that is cleared at the start of the render pass and stored at the end.
	VkAttachmentDescription colorAttachment = {
		.format         = swapchainImageFormat,
		.samples        = VK_SAMPLE_COUNT_1_BIT,
		.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	// The subpass describes the operations that will be performed on the attachments. In this case, we have one subpass that uses the color attachment.
	VkSubpassDescription subpass = {
		.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments    = &colorAttachmentRef,
	};

	VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments    = &colorAttachment,
		.subpassCount    = 1,
		.pSubpasses      = &subpass,
	};

	VkRenderPass renderPass;
	if (vkCreateRenderPass(device, &renderPassCreateInfo, NULL, &renderPass) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create render pass\n");
		return EXIT_FAILURE;
	}

	// Each swapchain image gets a framebuffer compatible with the render pass.
	VkFramebuffer *swapchainFramebuffers = malloc(sizeof(VkFramebuffer) * swapchainImageCount);
	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		VkImageView attachments[] = { swapchainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {
			.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass      = renderPass,
			.attachmentCount = 1,
			.pAttachments    = attachments,
			.width           = extent.width,
			.height          = extent.height,
			.layers          = 1,
		};

		if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapchainFramebuffers[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create framebuffer %u\n", i);
			return EXIT_FAILURE;
		}
	}

	// -----------------------------------------------------------------------------
	// Command buffer and synchronization setup
	// A command buffer is a sequence of commands that will be submitted to the GPU for execution.
	//
	// Make a reusable instruction list and a few traffic lights. The traffic lights
	// stop the GPU from using a picture before it is ready or showing it too early.
	// -----------------------------------------------------------------------------

	// Create a command pool for allocating command buffers. A command pool is a collection of command buffers that can be reset and reused.
	VkCommandPoolCreateInfo poolInfo = {
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = indices.graphicsFamily,
	};

	VkCommandPool commandPool;
	if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		return EXIT_FAILURE;
	}

	VkCommandBufferAllocateInfo allocInfo = {
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = commandPool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	// A semaphore is a synchronization primitive that can be used to coordinate the execution of command buffers.
	// A fence is a synchronization primitive that can be used to wait for the completion of command buffers.
	// The image-available semaphore gates rendering, the render-finished
	// semaphores gate presentation for their corresponding swapchain images, and
	// the fence keeps this frame in flight.
	VkSemaphore  imageAvailableSemaphore;
	VkSemaphore *renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * swapchainImageCount);
	VkFence      inFlightFence;

	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo     fenceInfo     = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

	vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore);
	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]);
	}
	vkCreateFence(device, &fenceInfo, NULL, &inFlightFence);

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

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		// Wait for the previous frame to finish before starting a new one.
		vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &inFlightFence);

		// Acquire the next image from the swapchain. This will block until an image is available, or return an error if the swapchain is out of date.
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result != VK_SUCCESS) {
			break;
		}

		// Record commands for the image acquired from the swapchain.
		vkResetCommandBuffer(commandBuffer, 0);

		// Begin recording commands into the command buffer. The command buffer will be submitted to the graphics queue for execution.
		VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		// Clear the framebuffer to a solid color.
		VkClearValue          clearColor          = { { { 0.5f, 0.0f, 0.5f, 1.0f } } }; // rgba
		VkRenderPassBeginInfo renderPassBeginInfo = {
			.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass        = renderPass,
			.framebuffer       = swapchainFramebuffers[imageIndex],
			.renderArea.offset = { 0, 0 },
			.renderArea.extent = extent,
			.clearValueCount   = 1,
			.pClearValues      = &clearColor,
		};

		// Begin the render pass and record the commands to clear the framebuffer.
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(commandBuffer);
		vkEndCommandBuffer(commandBuffer);

		// Submit the recorded work, then present the same swapchain image.
		VkSubmitInfo         submitInfo       = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
		VkSemaphore          waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[]     = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount         = 1;
		submitInfo.pWaitSemaphores            = waitSemaphores;
		submitInfo.pWaitDstStageMask          = waitStages;
		submitInfo.commandBufferCount         = 1;
		submitInfo.pCommandBuffers            = &commandBuffer;

		// Signal the render-finished semaphore when the command buffer has finished executing.
		VkSemaphore signalSemaphores[]  = { renderFinishedSemaphores[imageIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores    = signalSemaphores;

		// Submit the command buffer to the graphics queue for execution. The fence will be signaled when the command buffer has finished executing.
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);

		VkPresentInfoKHR presentInfo = {
			.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores    = signalSemaphores,
			.swapchainCount     = 1,
			.pSwapchains        = &swapchain,
			.pImageIndices      = &imageIndex,
		};

		vkQueuePresentKHR(graphicsQueue, &presentInfo); // Present the image to the screen.
	}

	// -----------------------------------------------------------------------------
	// Cleanup
	//
	// Wait for the GPU to finish, then destroy the things we created in reverse
	// order so nothing is still using something we already removed.
	// -----------------------------------------------------------------------------

	vkDeviceWaitIdle(device); // Wait for the device to finish all operations before cleaning up.

	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		vkDestroyFramebuffer(device, swapchainFramebuffers[i], NULL);
		vkDestroyImageView(device, swapchainImageViews[i], NULL);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
	}
	vkDestroyRenderPass(device, renderPass, NULL);
	vkDestroyCommandPool(device, commandPool, NULL);
	vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
	vkDestroyFence(device, inFlightFence, NULL);

	free(swapchainImages);
	free(swapchainFramebuffers);
	free(swapchainImageViews);
	free(renderFinishedSemaphores);
	free(swapchainSupport.formats);
	free(swapchainSupport.presentModes);

	vkDestroySwapchainKHR(device, swapchain, NULL);

	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);
	glfwDestroyWindow(window);
	glfwTerminate();

	printf("Cleanup complete. Exiting program.\n");

	return EXIT_SUCCESS;
}
