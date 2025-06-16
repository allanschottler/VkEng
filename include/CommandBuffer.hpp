#pragma once

struct CommandBuffer 
{
    Device& device;
    VkCommandBuffer commandBuffer;

    CommandBuffer(Device& device)
        : device(device), commandBuffer(VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo cbaInfo = {};
        cbaInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbaInfo.commandPool = device.commandPool;
        cbaInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbaInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device.logical, &cbaInfo, &commandBuffer) != VK_SUCCESS) {
            std::cerr << "Failed to allocate command buffer" << std::endl;
            exit(1);
        }
    }

    ~CommandBuffer()
    {
        vkFreeCommandBuffers(device.logical, device.commandPool, 1, &commandBuffer);
    }
};