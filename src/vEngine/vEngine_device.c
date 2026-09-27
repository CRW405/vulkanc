#include "vEngine.h"

#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Physical-device and queue-family helpers
// A Queue Family is a group of queues that support a specific set of operations.
// For example, a queue family may support graphics operations, while another may
// support compute operations. A physical device may have multiple queue families.
//
// Look at each GPU and find one that can draw pictures and show pictures.
// -----------------------------------------------------------------------------

QueueFamilyIndices vEngineFindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
	QueueFamilyIndices indices = vEngineFindQueueFamilies(device, surface);
	return indices.hasGraphicsFamily && indices.hasPresentFamily;
}

// -----------------------------------------------------------------------------
// Vulkan instance setup
// The instance is the connection between the application and the Vulkan library.
//
// Introduce our program to Vulkan, turn on the tools we need, and connect Vulkan
// to the window so Vulkan has a place to put pictures.
// -----------------------------------------------------------------------------
bool vEngineCreateInstance(vEngine *engine) {
	if (vEngineEnableValidationLayers && !vEngineCheckValidationLayerSupport()) {
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
	if (vEngineEnableValidationLayers) {
		createInfo.enabledLayerCount = vEngineValidationLayerCount;
		createInfo.ppEnabledLayerNames = vEngineValidationLayers;
	}

	if (vkCreateInstance(&createInfo, NULL, &engine->instance) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan instance\n");
		return false;
	}
	return true;
}

bool vEngineSelectPhysicalDevice(vEngine *engine) {
	// Pick the first GPU that provides both graphics and presentation queues.
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(engine->instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
		return false;
	}

	VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
	if (devices == NULL)
		return false;
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

// -----------------------------------------------------------------------------
// Logical device and queue setup
// A logical device is a handle to the physical device that allows us to interact with it.
// A queue is a command buffer that can be submitted to the GPU for execution.
//
// Choose the GPU's worker, give it a work line, and get that work line back so
// the program can send drawing jobs to the GPU.
// -----------------------------------------------------------------------------
bool vEngineCreateLogicalDevice(vEngine *engine, QueueFamilyIndices indices) {
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfos[2] = {
		{
         .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
         .queueFamilyIndex = indices.graphicsFamily,
         .queueCount = 1,
         .pQueuePriorities = &queuePriority,
		 }
	};
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
