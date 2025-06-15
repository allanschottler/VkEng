#pragma once
#include <stdexcept>
#include <vector>

template<class T>
struct Buffer {    
    using value_t = T;

    Device& device;
    std::size_t size;
    VkDeviceMemory memory;
    VkBuffer buffer;

    Buffer(Device& device, std::size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) 
        : device(device), size(size), memory(VK_NULL_HANDLE), buffer(VK_NULL_HANDLE)
    {
        std::cout << "creating buffer of size " << size << std::endl;
        VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(value_t) * size,
            .usage = usage };

        if(vkCreateBuffer(device.logical, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create buffer");

        std::cout << "created buffer of size " << size << std::endl;
        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device.logical, buffer, &memReqs);

        std::cout << "buffer memory requirements: size = " << memReqs.size 
                  << ", alignment = " << memReqs.alignment 
                  << ", memoryTypeBits = " << memReqs.memoryTypeBits << std::endl;

        VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReqs.size };
        
        if(!getMemoryType(memReqs.memoryTypeBits, properties, &allocInfo.memoryTypeIndex))
            throw std::runtime_error("failed to find suitable memory type");

        std::cout << "found suitable memory type with index " << allocInfo.memoryTypeIndex << std::endl;

        if(vkAllocateMemory(device.logical, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate buffer memory");

        std::cout << "allocated buffer memory of size " << memReqs.size 
                  << " with memory type index " << allocInfo.memoryTypeIndex << std::endl;
        vkBindBufferMemory(device.logical, buffer, memory, 0);
        
        std::cout << "bound buffer memory to buffer" << std::endl;
    }

    ~Buffer() {
        vkDestroyBuffer(device.logical, buffer, nullptr);
        vkFreeMemory(device.logical, memory, nullptr);
    }

private:
    VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex) {
		for (uint32_t i = 0; i < 32; i++) {
			if ((typeBits & 1) == 1) {
				if ((device.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					*typeIndex = i;
					return true;
				}
			}
			typeBits >>= 1;
		}
		return false;
	}
};

template<class T>
struct DeviceBuffer : Buffer<T> {
    DeviceBuffer(Device& device, std::size_t size, VkBufferUsageFlags usage)
        : Buffer<T>(device, size,
            usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}
};

// struct scoped_memory_map {
//     Device& device;
//     VkDeviceMemory memory;
//     void* data;

//     scoped_memory_map(Device& device, VkDeviceMemory memory, std::size_t size)
//         : device(device), memory(memory), data(nullptr) 
//     {
//         vkMapMemory(device.logical, memory, 0, size, 0, &data);
//     }

//     ~scoped_memory_map() {
//         if (data) {
//             vkUnmapMemory(device.logical, memory);
//         }
//     }

//     void* get() { return data; }
// };

// struct scoped_memory_copy : scoped_memory_map {
//     template<class It, class Sentinel = It>
//     scoped_memory_copy(Device& device, VkDeviceMemory memory, It begin, Sentinel end)
//         : scoped_memory_map(device, memory, sizeof(*begin) * std::distance(begin, end))
//     { memcpy(data, src, size); }
// };

template<class T>
struct HostBuffer : Buffer<T> {
    using base = Buffer<T>;
    using value_t = typename base::value_t;

    template<class It, class Sentinel = It>
    HostBuffer(Device& device, It begin, Sentinel end, VkBufferUsageFlags usage)
        requires(std::is_same_v<typename std::iterator_traits<It>::value_type, value_t>)
        : Buffer<T>(device, std::distance(begin, end), usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) 
    { map(begin, end); }
    
    HostBuffer(Device& device, const std::vector<T>& data, VkBufferUsageFlags usage)
        : HostBuffer<T>(device, data.begin(), data.end(), usage) {}

    template<class It, class Sentinel = It>
    void map(It begin, Sentinel end) 
        requires(std::is_same_v<typename std::iterator_traits<It>::value_type, value_t>) 
    {
        if(static_cast<std::size_t>(std::distance(begin, end)) != base::size)
            throw std::runtime_error("Size mismatch when mapping host buffer");
        if(begin == end)
            throw std::runtime_error("Cannot create host buffer with empty range");
        void* mappedData;
        vkMapMemory(base::device.logical, base::memory, 0, sizeof(value_t) * base::size, 0, &mappedData);
        memcpy(mappedData, &*begin, sizeof(value_t) * base::size);
        vkUnmapMemory(base::device.logical, base::memory);
    }
};

template<class T>
struct StagingBuffer : HostBuffer<T> {
    StagingBuffer(Device& device, const std::vector<T>& data)
        : HostBuffer<T>(device, data, VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {}
};

template<class T>
struct UniformBuffer : HostBuffer<T> {
    UniformBuffer(Device& device, const T& data)
        : HostBuffer<T>(device, &data, &data + 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {}
    
    void map(const T& data) 
    { HostBuffer<T>::map(&data, &data + 1); }
};