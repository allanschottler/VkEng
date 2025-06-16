#pragma once

struct RenderPass
{
    Device& device;
    SwapChain& swapChain;
    VkRenderPass renderPass;

    RenderPass(Device& device, SwapChain& swapChain)
        : device(device), swapChain(swapChain), renderPass(VK_NULL_HANDLE)
    {
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.format = swapChain.format;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// Note: hardware will automatically transition attachment to the specified layout
		// Note: index refers to attachment descriptions array
		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Note: this is a description of how the attachments of the render pass will be used in this sub pass
		// e.g. if they will be read in shaders and/or drawn to
		VkSubpassDescription subPassDescription = {};
		subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPassDescription.colorAttachmentCount = 1;
		subPassDescription.pColorAttachments = &colorAttachmentReference;

		// Create the render pass
		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &attachmentDescription;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPassDescription;

		if (vkCreateRenderPass(device.logical, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
			std::cerr << "failed to create render pass" << std::endl;
			exit(1);
		} else {
			std::cout << "created render pass" << std::endl;
		}
	}

    ~RenderPass()
    {
        vkDestroyRenderPass(device.logical, renderPass, nullptr);
    }
};
