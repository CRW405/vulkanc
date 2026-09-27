#include "vEngine.h"

// -----------------------------------------------------------------------------
// Vertex buffer creation
//
// A vertex buffer is a GPU resource that holds vertex data. The vertex data is
// used by the vertex shader to generate the final position of each vertex in
// screen space. The vertex buffer is created from a set of vertices, which
// are copied into GPU memory. The vertex buffer is then bound to the graphics
// pipeline so that the vertex shader can access the vertex data.
// -----------------------------------------------------------------------------

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	fprintf(stderr, "Failed to find suitable memory type!\n");
	return 0;
}

bool vEngineCreateVertexBuffer(vEngine *engine, const Vertex *vertices, uint32_t vertexCount) {
	if (engine == NULL || vertices == NULL || vertexCount == 0 ||
	    engine->device == VK_NULL_HANDLE || engine->physicalDevice == VK_NULL_HANDLE)
		return false;

	VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount;

	// 1. Create Buffer Object
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	if (vkCreateBuffer(engine->device, &bufferInfo, NULL, &engine->vertexBuffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create vertex buffer\n");
		return false;
	}

	// 2. Get Memory Requirements
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(engine->device, engine->vertexBuffer, &memRequirements);

	// 3. Allocate Memory
	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(engine->physicalDevice, memRequirements.memoryTypeBits,
		                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	};

	if (vkAllocateMemory(engine->device, &allocInfo, NULL, &engine->vertexBufferMemory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate vertex buffer memory\n");
		vkDestroyBuffer(engine->device, engine->vertexBuffer, NULL);
		engine->vertexBuffer = VK_NULL_HANDLE;
		return false;
	}

	// 4. Bind Memory to Buffer
	if (vkBindBufferMemory(engine->device, engine->vertexBuffer, engine->vertexBufferMemory, 0) != VK_SUCCESS) {
		fprintf(stderr, "Failed to bind vertex buffer memory\n");
		vkFreeMemory(engine->device, engine->vertexBufferMemory, NULL);
		vkDestroyBuffer(engine->device, engine->vertexBuffer, NULL);
		engine->vertexBufferMemory = VK_NULL_HANDLE;
		engine->vertexBuffer = VK_NULL_HANDLE;
		return false;
	}

	// 5. Copy Vertex Data into GPU Memory
	void *data;
	if (vkMapMemory(engine->device, engine->vertexBufferMemory, 0, bufferSize, 0, &data) != VK_SUCCESS) {
		fprintf(stderr, "Failed to map vertex buffer memory\n");
		vkFreeMemory(engine->device, engine->vertexBufferMemory, NULL);
		vkDestroyBuffer(engine->device, engine->vertexBuffer, NULL);
		engine->vertexBufferMemory = VK_NULL_HANDLE;
		engine->vertexBuffer = VK_NULL_HANDLE;
		return false;
	}
	memcpy(data, vertices, (size_t)bufferSize);
	vkUnmapMemory(engine->device, engine->vertexBufferMemory);
	engine->vertexCount = vertexCount;

	printf("Vertex buffer created and populated successfully\n");
	return true;
}
