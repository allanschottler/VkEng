#pragma once

struct ImageView
{
    Device& device;
    SwapChain& swapChain;
    VkImageView imageView;

    ImageView(Device& device, SwapChain& swapChain, VkImage image)
        : device(device), swapChain(swapChain), imageView(VK_NULL_HANDLE)
    {
        std::cout << "creating image view" << std::endl;

        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChain.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.logical, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
            std::cerr << "failed to create image view" << std::endl;
            exit(1);
        } else {
            std::cout << "created image view" << std::endl;
        }
    }

    ~ImageView()
    {
        vkDestroyImageView(device.logical, imageView, nullptr);
    }
};