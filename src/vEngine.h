#ifndef VENGINE_H
#define VENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <stdbool.h>

#define VENGINE_WINDOW_WIDTH 500
#define VENGINE_WINDOW_HEIGHT 500

typedef struct vEngine {
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	uint32_t swapchainImageCount;
	VkImage *swapchainImages;
	VkImageView *swapchainImageViews;
	VkFramebuffer *swapchainFramebuffers;

	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore *renderFinishedSemaphores;
	VkFence inFlightFence;
} vEngine;

bool vEngineCreate(vEngine *engine, GLFWwindow *window);
void vEngineDrawFrame(vEngine *engine);
void vEngineDestroy(vEngine *engine);

#endif
