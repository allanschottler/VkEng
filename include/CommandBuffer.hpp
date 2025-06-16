#pragma once

template<class... Policies>
struct CommandBuffer 
{
    Device& device;
    VkCommandBuffer commandBuffer;

    CommandBuffer(Device& device, VkCommandBufferUsageFlagBits usage, Policies&&... policies)
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

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = usage;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            std::cerr << "Failed to begin command buffer" << std::endl;
            exit(1);
        }

        std::apply([this](auto&&... policy) {
            (policy.cmd(commandBuffer), ...);
        }, std::make_tuple(std::forward<Policies>(policies)...));

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            std::cerr << "Failed to end command buffer" << std::endl;
            exit(1);
        }
    }

    ~CommandBuffer()
    {
        vkFreeCommandBuffers(device.logical, device.commandPool, 1, &commandBuffer);
    }
};

template<class T>
struct CmdCopyBuffer
{
    HostBuffer<T>& srcBuffer;
    DeviceBuffer<T>& dstBuffer;
    VkBufferCopy copyRegion{};

    CmdCopyBuffer(HostBuffer<T>& srcBuffer, DeviceBuffer<T>& dstBuffer)
        : srcBuffer(srcBuffer), dstBuffer(dstBuffer)
    {
        copyRegion.size = std::min(srcBuffer.size, dstBuffer.size) * sizeof(T);
    }    

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        std::cout << "Copying buffer from host to device with size " << copyRegion.size << std::endl;
        vkCmdCopyBuffer(cmdBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);
    }
};