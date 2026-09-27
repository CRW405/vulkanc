#include "vEngine.h"

#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Render pass and framebuffer setup
// A render pass describes the attachments and operations used for one draw.
// A framebuffer is a collection of attachments that are used as the destination for rendering.
//
// Tell Vulkan, "For each picture, start by clearing it, let drawing happen,
// then leave the finished color ready for the screen." Build one framebuffer
// for each swapchain picture.
// -----------------------------------------------------------------------------
bool vEngineCreateRenderPassAndFramebuffers(vEngine *engine) {
	VkAttachmentDescription colorAttachment = {
		.format = engine->swapchainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
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
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = engine->renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = engine->swapchainExtent.width,
			.height = engine->swapchainExtent.height,
			.layers = 1,
		};
		if (vkCreateFramebuffer(engine->device, &framebufferInfo, NULL, &engine->swapchainFramebuffers[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create framebuffer %u\n", i);
			return false;
		}
	}
	return true;
}

// -----------------------------------------------------------------------------
// Shaders
//
// A shader is a small program that runs on the GPU. Shaders are written in GLSL, then
// compiled to SPIR-V, which is a binary format that Vulkan can understand. The shader
// is loaded into the program as a byte array, then passed to Vulkan to create a shader
// module.
// -----------------------------------------------------------------------------

static char *readFile(const char *filename, size_t *outSize) {
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

static VkShaderModule createShaderModule(VkDevice device, const char *code, size_t size) {
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

bool vEngineCreateGraphicsPipeline(vEngine *engine) {
	size_t vertSize = 0, fragSize = 0;
	char *vertCode = readFile("./shaders/triangle.vert.spv", &vertSize);
	char *fragCode = readFile("./shaders/triangle.frag.spv", &fragSize);
	if (vertCode == NULL || fragCode == NULL) {
		free(vertCode);
		free(fragCode);
		return false;
	}

	VkShaderModule vertShaderModule = createShaderModule(engine->device, vertCode, vertSize);
	VkShaderModule fragShaderModule = createShaderModule(engine->device, fragCode, fragSize);
	free(vertCode);
	free(fragCode);
	if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
		return false;
	}

	// a shader stage is a single programmable stage in the graphics pipeline, such as vertex or fragment
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main",
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main",
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// a vertex input state describes the format of the vertex data that will be passed to the vertex shader
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.vertexAttributeDescriptionCount = 0,
	};

	// a input assembly state describes how the vertices will be assembled into primitives (e.g. triangles, lines, points)
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	// a viewport state describes the region of the framebuffer that will be rendered to
	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)engine->swapchainExtent.width,
		.height = (float)engine->swapchainExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	// a scissor state describes the region of the framebuffer that will be affected by rendering
	VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = engine->swapchainExtent,
	};

	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};

	// a rasterization state describes how the primitives will be rasterized into fragments
	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
	};

	// a multisample state describes how the fragments will be sampled and combined
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	// a color blend state describes how the fragments will be blended with the existing framebuffer contents
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
	};
	VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	};
	if (vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, NULL, &engine->pipelineLayout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create pipeline layout\n");
		vkDestroyShaderModule(engine->device, vertShaderModule, NULL);
		vkDestroyShaderModule(engine->device, fragShaderModule, NULL);
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pColorBlendState = &colorBlending,
		.layout = engine->pipelineLayout,
		.renderPass = engine->renderPass,
		.subpass = 0,
	};

	if (vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &engine->graphicsPipeline) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create graphics pipeline\n");
		vkDestroyShaderModule(engine->device, vertShaderModule, NULL);
		vkDestroyShaderModule(engine->device, fragShaderModule, NULL);
		return false;
	}

	vkDestroyShaderModule(engine->device, vertShaderModule, NULL);
	vkDestroyShaderModule(engine->device, fragShaderModule, NULL);
	printf("Graphics pipeline created successfully\n");
	return true;
}
