#pragma once

template<class T>
struct Buffer {
    VkDevice& device;
    VkDeviceMemory memory;
    VkBuffer buffer;
    std::size_t size;

    Buffer(VkDevice& device, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::size_t size) 
        : device(device), memory(VK_NULL_HANDLE), buffer(VK_NULL_HANDLE), size(size)
    {
        VkBufferCreateInfo bufferInfo;
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(T) * size;
        bufferInfo.usage = usage;
        // bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create buffer");

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, buffer, &memReqs);

        VkMemoryAllocateInfo allocInfo;
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;

        // if(!getMemoryType(memReqs.memoryTypeBits, properties, &allocInfo.memoryTypeIndex))
        //     throw std::runtime_error("failed to find suitable memory type");

        if(vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate buffer memory");

        vkBindBufferMemory(device, buffer, memory, 0);       
    }

    ~Buffer() {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }

// private:
//     VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex) {
// 		for (uint32_t i = 0; i < 32; i++) {
// 			if ((typeBits & 1) == 1) {
// 				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
// 					*typeIndex = i;
// 					return true;
// 				}
// 			}
// 			typeBits >>= 1;
// 		}
// 		return false;
// 	}
};

template<class T>
struct DeviceBuffer : public Buffer<T> {
    DeviceBuffer(VkDevice& device, std::size_t size, VkBufferUsageFlags usage)
        : Buffer<T>(device, size,
            usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}
};

struct scoped_memory_map {
    VkDevice& device;
    VkDeviceMemory memory;
    void* data;

    scoped_memory_map(VkDevice& device, VkDeviceMemory memory, std::size_t size)
        : device(device), memory(memory), data(nullptr) 
    {
        vkMapMemory(device, memory, 0, size, 0, &data);
    }

    ~scoped_memory_map() {
        if (data) {
            vkUnmapMemory(device, memory);
        }
    }

    void* get() { return data; }
};

// struct scoped_memory_copy : scoped_memory_map {
//     template<class It, class Sentinel = It>
//     scoped_memory_copy(VkDevice& device, VkDeviceMemory memory, It begin, Sentinel end)
//         : scoped_memory_map(device, memory, sizeof(*begin) * std::distance(begin, end))
//     { memcpy(data, src, size); }
// };

template<class T>
struct HostBuffer : public Buffer<T> {
    template<class It, class Sentinel = It>
    HostBuffer(VkDevice& device, It begin, Sentinel end)
        : Buffer<T>(device, std::distance(begin, end),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) 
    {
        map(begin, end);
    }
    HostBuffer(VkDevice& device, const std::vector<T>& data)
        : HostBuffer<T>(device, data.begin(), data.end()) {}

    template<class It, class Sentinel = It>
    void map(It begin, Sentinel end) {
        if(static_cast<std::size_t>(std::distance(begin, end)) != this->size)
            throw std::runtime_error("Size mismatch when mapping host buffer");
        void* mappedData;
        vkMapMemory(this->device, this->memory, 0, sizeof(T) * this->size, 0, &mappedData);
        memcpy(mappedData, &*begin, this->size);
        vkUnmapMemory(this->device, this->memory);
    }
};