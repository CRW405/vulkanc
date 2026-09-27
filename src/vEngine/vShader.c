#include "vEngine.h"

// -----------------------------------------------------------------------------
// Shaders
//
// A shader is a small program that runs on the GPU. Shaders are written in GLSL, then
// compiled to SPIR-V, which is a binary format that Vulkan can understand. The shader
// is loaded into the program as a byte array, then passed to Vulkan to create a shader
// module.
// -----------------------------------------------------------------------------

char *readFile(const char *filename, size_t *outSize) {
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		fprintf(stderr, "Failed to seek file: %s\n", filename);
		fclose(file);
		return NULL;
	}
	long fileSize = ftell(file);
	if (fileSize < 0) {
		fprintf(stderr, "Failed to determine size for file: %s\n", filename);
		fclose(file);
		return NULL;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to rewind file: %s\n", filename);
		fclose(file);
		return NULL;
	}

	char *buffer = malloc((size_t)fileSize);
	if (buffer == NULL) {
		fprintf(stderr, "Failed to allocate memory for file: %s\n", filename);
		fclose(file);
		return NULL;
	}
	const size_t readBytes = fread(buffer, 1, (size_t)fileSize, file);
	if (readBytes != (size_t)fileSize) {
		fprintf(stderr, "Failed to read file: %s\n", filename);
		free(buffer);
		fclose(file);
		return NULL;
	}
	fclose(file);
	*outSize = (size_t)fileSize;
	return buffer;
}

VkShaderModule createShaderModule(VkDevice device, const char *code, size_t size) {
	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = (const uint32_t *)code,
	};
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create shader module\n");
		return VK_NULL_HANDLE;
	}
	return shaderModule;
}
