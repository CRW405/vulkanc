#include "vEngine.h"

// -----------------------------------------------------------------------------
// Vertex input descriptions
//
// The vertex input descriptions describe how the vertex data is laid out in memory
// and how it will be passed to the vertex shader. The binding description describes
// the rate at which vertex data is consumed, and the attribute descriptions describe
// the format of each vertex attribute (position, color, etc.).
// -----------------------------------------------------------------------------

VkVertexInputBindingDescription vEngineGetVertexInputBindingDescription(void) {
	VkVertexInputBindingDescription bindingDescription = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	return bindingDescription;
}

void vEngineGetVertexAttributeDescriptions(VkVertexInputAttributeDescription *attributeDescriptions) {
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);
}

// -----------------------------------------------------------------------------
// Graphics pipeline setup
// A graphics pipeline describes the steps that the GPU will take to render a frame.
// The pipeline is created from a set of shader stages, fixed-function stages, and
// pipeline state objects.
// -----------------------------------------------------------------------------

bool vEngineCreateGraphicsPipeline(vEngine *engine, const char *vertexShaderPath, const char *fragmentShaderPath) {
	size_t vertSize = 0, fragSize = 0;
	char *vertCode = readFile(vertexShaderPath, &vertSize);
	char *fragCode = readFile(fragmentShaderPath, &fragSize);
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
	VkVertexInputBindingDescription bindingDescription = vEngineGetVertexInputBindingDescription();
	VkVertexInputAttributeDescription attributeDescriptions[2];
	vEngineGetVertexAttributeDescriptions(attributeDescriptions);
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = attributeDescriptions,
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

	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(float) * 16,
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
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
