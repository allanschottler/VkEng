#pragma once

struct Framebuffer
{
    Device& device;
    SwapChain& swapChain;
    VkFramebuffer framebuffer;

    Framebuffer(Device& device, SwapChain& swapChain, ImageView& imageView, RenderPass& renderPass)
        : device(device), swapChain(swapChain), framebuffer(VK_NULL_HANDLE)
    {
        std::cout << "creating framebuffer" << std::endl;

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass.renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &imageView.imageView;
        createInfo.width = swapChain.extent.width;
        createInfo.height = swapChain.extent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(device.logical, &createInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            std::cerr << "failed to create framebuffer" << std::endl;
            exit(1);
        } else {
            std::cout << "created framebuffer" << std::endl;
        }
    }

    ~Framebuffer()
    {
        vkDestroyFramebuffer(device.logical, framebuffer, nullptr);
    }
};