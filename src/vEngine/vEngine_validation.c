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

const bool vEngineEnableValidationLayers = true;
const char *vEngineValidationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};
const uint32_t vEngineValidationLayerCount = sizeof(vEngineValidationLayers) / sizeof(vEngineValidationLayers[0]);

bool vEngineCheckValidationLayerSupport(void) {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
	if (availableLayers == NULL) {
		return false;
	}
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	for (uint32_t i = 0; i < vEngineValidationLayerCount; i++) {
		bool layerFound = false;
		for (uint32_t j = 0; j < layerCount; j++) {
			if (strcmp(vEngineValidationLayers[i], availableLayers[j].layerName) == 0) {
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
