#pragma once

struct CommandBufferBase
{
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};

    virtual ~CommandBufferBase() = default;
    virtual void use(VkCommandBufferUsageFlagBits usage) {};
};

template<class... Policies>
struct CommandBuffer : CommandBufferBase
{
    Device& device;
    std::tuple<Policies...> policies;
    
    CommandBuffer(Device& device, Policies&&... policies)
        : CommandBufferBase(), device(device), policies(std::forward<Policies>(policies)...)        
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

    void use(VkCommandBufferUsageFlagBits usage) override
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = usage;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            std::cerr << "Failed to begin command buffer" << std::endl;
            exit(1);
        }

        std::apply([this](auto&&... policy) {
            (policy.cmd(commandBuffer), ...);
        }, policies);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            std::cerr << "Failed to end command buffer" << std::endl;
            exit(1);
        }
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

struct CmdBeginRenderPass
{
    SwapChain& swapChain;
    RenderPass& renderPass;
    Framebuffer& framebuffer;
    VkRenderPassBeginInfo beginInfo{};

    CmdBeginRenderPass(SwapChain& swapChain, RenderPass& renderPass, Framebuffer& framebuffer, VkClearValue& clearValue)
        : swapChain(swapChain), renderPass(renderPass), framebuffer(framebuffer)
    {
			beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginInfo.renderPass = renderPass.renderPass;
			beginInfo.framebuffer = framebuffer.framebuffer;
			beginInfo.renderArea.offset.x = 0;
			beginInfo.renderArea.offset.y = 0;
			beginInfo.renderArea.extent = swapChain.extent;
			beginInfo.clearValueCount = 1;
			beginInfo.pClearValues = &clearValue;
    }

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
};

struct CmdEndRenderPass
{
    void cmd(VkCommandBuffer& cmdBuffer)
    {
        vkCmdEndRenderPass(cmdBuffer);
    }
};

template<class T>
struct CmdBindVertexBuffer
{
    DeviceBuffer<T>& vertexBuffer;

    CmdBindVertexBuffer(DeviceBuffer<T>& vertexBuffer)
        : vertexBuffer(vertexBuffer) {}

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);
    }
};

struct CmdBindIndexBuffer
{
    DeviceBuffer<uint32_t>& indexBuffer;

    CmdBindIndexBuffer(DeviceBuffer<uint32_t>& indexBuffer)
        : indexBuffer(indexBuffer) {}

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }
};

struct CmdDrawIndexed
{
    uint32_t indexCount;
    uint32_t instanceCount;
    int32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;

    //TODO: pass index buffer as argument
    CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, int32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0)
        : indexCount(indexCount), instanceCount(instanceCount), firstIndex(firstIndex), vertexOffset(vertexOffset), firstInstance(firstInstance) {}

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        vkCmdDrawIndexed(cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
};

struct CmdBindPipeline
{
    GraphicsPipeline& pipeline;
    VkPipelineBindPoint bindPoint;

    CmdBindPipeline(GraphicsPipeline& pipeline, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS)
        : pipeline(pipeline), bindPoint(bindPoint) {}

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        vkCmdBindPipeline(cmdBuffer, bindPoint, pipeline.pipeline);
    }
};

struct CmdBindDescriptorSets
{
    GraphicsPipeline& pipeline;
    VkDescriptorSet& descriptorSet;
    VkPipelineBindPoint bindPoint;

    CmdBindDescriptorSets(GraphicsPipeline& pipeline, VkDescriptorSet& descriptorSet, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS)
        : pipeline(pipeline), descriptorSet(descriptorSet), bindPoint(bindPoint) {}

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        vkCmdBindDescriptorSets(cmdBuffer, bindPoint, pipeline.pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }
};

struct CmdPipelineBarrier
{
    VkImageMemoryBarrier barrier{};
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;

    CmdPipelineBarrier(Device& device, SwapChain& swapChain, VkImage& image,
                       VkImageLayout oldLayout, VkImageLayout newLayout,
                       VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                       VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                       uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex)
        : srcStageMask(srcStageMask), dstStageMask(dstStageMask)
    {
        VkImageSubresourceRange subResourceRange = {};
		subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResourceRange.baseMipLevel = 0;
		subResourceRange.levelCount = 1;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.layerCount = 1;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
        barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
        barrier.image = image;
        barrier.subresourceRange = subResourceRange;
    }
    
    void cmd(VkCommandBuffer& cmdBuffer)
    {
        vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
};

struct CmdPresent2GraphicsBarrier
{
    CmdPipelineBarrier barrier;

    CmdPresent2GraphicsBarrier(Device& device, SwapChain& swapChain, VkImage& image)
        : barrier(device, swapChain, image,
                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                  0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
                  VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED)
    {
        if (device.presentQueueFamily == device.graphicsQueueFamily) 
        {
            barrier.barrier.srcQueueFamilyIndex = device.presentQueueFamily;
            barrier.barrier.dstQueueFamilyIndex = device.graphicsQueueFamily;
        }
    }

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        barrier.cmd(cmdBuffer);
    }
};

struct CmdGraphics2PresentBarrier
{
    Device& device;
    CmdPipelineBarrier barrier;

    CmdGraphics2PresentBarrier(Device& device, SwapChain& swapChain, VkImage& image)
        : device(device), barrier(device, swapChain, image,
                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, 
                  device.graphicsQueueFamily, device.presentQueueFamily)
    {
    }

    void cmd(VkCommandBuffer& cmdBuffer)
    {
        if (barrier.barrier.srcQueueFamilyIndex != barrier.barrier.dstQueueFamilyIndex)
            barrier.cmd(cmdBuffer);
    }
};