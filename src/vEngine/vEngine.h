#ifndef VENGINE_H
#define VENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Engine state
//
// Holds all Vulkan handles and resources owned by one engine instance.
// ============================================================================

typedef struct vEngine {
	// Step 1: Vulkan instance and presentation surface.
	VkInstance instance;
	VkSurfaceKHR surface;

	// Step 2: Selected GPU and logical device queues.
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// Step 3: Swapchain images and the views/framebuffers used to present them.
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	uint32_t swapchainImageCount;
	VkImage *swapchainImages;
	VkImageView *swapchainImageViews;
	VkFramebuffer *swapchainFramebuffers;

	// Step 4: Render pass and command recording resources.
	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	// Step 5: Shader-backed graphics pipeline and vertex buffers.
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	uint32_t vertexCount;

	// Step 6: Per-frame synchronization and render clear color.
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore *renderFinishedSemaphores;
	VkFence inFlightFence;
	VkClearValue clearColor;
} vEngine;

// ============================================================================
// Vulkan setup data
// ============================================================================

// Queue families discovered on a physical device.
typedef struct QueueFamilyIndices {
	uint32_t graphicsFamily;
	bool hasGraphicsFamily;
	uint32_t presentFamily;
	bool hasPresentFamily;
} QueueFamilyIndices;

// Swapchain capabilities reported by a physical device and surface.
typedef struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	uint32_t formatCount;
	VkSurfaceFormatKHR *formats;
	uint32_t presentModeCount;
	VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

// Vertex layout consumed by the graphics pipeline.
typedef struct Vertex {
	float position[3];
	float color[3];
} Vertex;

// ============================================================================
// Public engine lifecycle and rendering API
// ============================================================================

// Creates the core Vulkan resources in setup order.
bool vEngineCreate(vEngine *engine, GLFWwindow *window);

// Loads shader files and creates the graphics pipeline.
bool vEngineLoadShaders(vEngine *engine, const char *vertexShaderPath, const char *fragmentShaderPath);

// Sets the color used to clear the render target each frame.
void vEngineSetClearColor(vEngine *engine, float red, float green, float blue, float alpha);

// Acquires a swapchain image, records/submits commands, and presents it.
void vEngineDrawFrame(vEngine *engine);

// Releases all Vulkan resources owned by the engine.
void vEngineDestroy(vEngine *engine);

// ============================================================================
// Validation configuration
// ============================================================================

extern const bool vEngineEnableValidationLayers;
extern const char *vEngineValidationLayers[];
extern const uint32_t vEngineValidationLayerCount;

// ============================================================================
// Vulkan initialization steps
// ============================================================================

// Step 1: Checks whether requested validation layers are available.
bool vEngineCheckValidationLayerSupport(void);

// Step 1: Creates the Vulkan instance and window surface.
bool vEngineCreateInstance(vEngine *engine);

// Step 2: Finds graphics and presentation queue families for a physical device.
QueueFamilyIndices vEngineFindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

// Step 2: Selects a suitable physical device (GPU).
bool vEngineSelectPhysicalDevice(vEngine *engine);

// Step 2: Creates the logical device and retrieves its queues.
bool vEngineCreateLogicalDevice(vEngine *engine, QueueFamilyIndices indices);

// Step 3: Creates swapchain images and their image views.
bool vEngineCreateSwapchain(vEngine *engine, GLFWwindow *window, QueueFamilyIndices indices);

// Step 4: Creates the render pass and one framebuffer per swapchain image.
bool vEngineCreateRenderPassAndFramebuffers(vEngine *engine);

// Step 4: Creates command pool/buffer resources and frame synchronization.
bool vEngineCreateCommandResources(vEngine *engine, uint32_t graphicsFamily);

// Step 5: Loads shaders and creates the pipeline used for drawing.
bool vEngineCreateGraphicsPipeline(vEngine *engine, const char *vertexShaderPath, const char *fragmentShaderPath);

// Step 5: Describes the vertex layout consumed by the graphics pipeline.
VkVertexInputBindingDescription vEngineGetVertexInputBindingDescription(void);
void vEngineGetVertexAttributeDescriptions(VkVertexInputAttributeDescription *attributeDescriptions);

// Step 5: Creates and uploads vertex data for the graphics pipeline.
bool vEngineCreateVertexBuffer(vEngine *engine, const Vertex *vertices, uint32_t vertexCount);

// ============================================================================
// Shader file and module helpers
// ============================================================================

// Reads a binary shader file and returns its contents and size.
char *readFile(const char *filename, size_t *outSize);

// Creates a Vulkan shader module from SPIR-V bytecode.
VkShaderModule createShaderModule(VkDevice device, const char *code, size_t size);

#endif
