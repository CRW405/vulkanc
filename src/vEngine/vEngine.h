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

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore *renderFinishedSemaphores;
	VkFence inFlightFence;
} vEngine;

typedef struct QueueFamilyIndices {
	uint32_t graphicsFamily;
	bool hasGraphicsFamily;
	uint32_t presentFamily;
	bool hasPresentFamily;
} QueueFamilyIndices;

typedef struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	uint32_t formatCount;
	VkSurfaceFormatKHR *formats;
	uint32_t presentModeCount;
	VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

bool vEngineCreate(vEngine *engine, GLFWwindow *window);
void vEngineDrawFrame(vEngine *engine);
void vEngineDestroy(vEngine *engine);

extern const bool vEngineEnableValidationLayers;
extern const char *vEngineValidationLayers[];
extern const uint32_t vEngineValidationLayerCount;

bool vEngineCheckValidationLayerSupport(void);
bool vEngineCreateInstance(vEngine *engine);
QueueFamilyIndices vEngineFindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
bool vEngineSelectPhysicalDevice(vEngine *engine);
bool vEngineCreateLogicalDevice(vEngine *engine, QueueFamilyIndices indices);
bool vEngineCreateSwapchain(vEngine *engine, QueueFamilyIndices indices);
bool vEngineCreateRenderPassAndFramebuffers(vEngine *engine);
bool vEngineCreateCommandResources(vEngine *engine, uint32_t graphicsFamily);
bool vEngineCreateGraphicsPipeline(vEngine *engine);

#endif
