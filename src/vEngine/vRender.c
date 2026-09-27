#include "vEngine.h"

// -----------------------------------------------------------------------------
// Render pass and framebuffer setup
// A render pass describes the attachments and operations used for one draw.
// A framebuffer is a collection of attachments that are used as the destination for rendering.
//
// Tell Vulkan, "For each picture, start by clearing it, let drawing happen,
// then leave the finished color ready for the screen." Build one framebuffer
// for each swapchain picture.
// -----------------------------------------------------------------------------
//
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
