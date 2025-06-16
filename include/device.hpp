#pragma once
#include <stdexcept>
#include <iostream>
#include <vector>

struct Device {
    Instance& instance;
	VkSurfaceKHR& windowSurface;
    VkDevice logical;
    VkPhysicalDevice physical;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    uint32_t graphicsQueueFamily;
	uint32_t presentQueueFamily;

    Device(Instance& instance, VkSurfaceKHR& windowSurface)
        : instance(instance)
        , windowSurface(windowSurface)
    {
        findPhysicalDevice();
		checkSwapChainSupport();
		findQueueFamilies();
        float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo[2] = {};

		queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamily;
		queueCreateInfo[0].queueCount = 1;
		queueCreateInfo[0].pQueuePriorities = &queuePriority;

		queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[0].queueFamilyIndex = presentQueueFamily;
		queueCreateInfo[0].queueCount = 1;
		queueCreateInfo[0].pQueuePriorities = &queuePriority;

		// Create logical device from physical device
		// Note: there are separate instance and device extensions!
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

		if (graphicsQueueFamily == presentQueueFamily) {
			deviceCreateInfo.queueCreateInfoCount = 1;
		} else {
			deviceCreateInfo.queueCreateInfoCount = 2;
		}

		// Necessary for shader (for some reason)
		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.shaderClipDistance = VK_TRUE;
		enabledFeatures.shaderCullDistance = VK_TRUE;

		const char* deviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = &deviceExtensions;
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

		if (vkCreateDevice(physical, &deviceCreateInfo, nullptr, &logical) != VK_SUCCESS)
			throw std::runtime_error("failed to create logical device");

		std::cout << "created logical device" << std::endl;

		// Get graphics and presentation queues (which may be the same)
		vkGetDeviceQueue(logical, graphicsQueueFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(logical, presentQueueFamily, 0, &presentQueue);

		std::cout << "acquired graphics and presentation queues" << std::endl;

		vkGetPhysicalDeviceMemoryProperties(physical, &memoryProperties);
    }

    ~Device() {
        vkDestroyDevice(logical, nullptr);
    }

    void findPhysicalDevice() {
		// Try to find 1 Vulkan supported device
		// Note: perhaps refactor to loop through devices and find first one that supports all required features and extensions
		uint32_t deviceCount = 0;
		if (vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
			std::cerr << "failed to get number of physical devices" << std::endl;
			exit(1);
		}

		deviceCount = 1;
		VkResult res = vkEnumeratePhysicalDevices(instance.instance, &deviceCount, &physical);
		if (res != VK_SUCCESS && res != VK_INCOMPLETE) {
			std::cerr << "enumerating physical devices failed!" << std::endl;
			exit(1);
		}

		if (deviceCount == 0) {
			std::cerr << "no physical devices that support vulkan!" << std::endl;
			exit(1);
		}

		std::cout << "physical device with vulkan support found" << std::endl;

		// Check device features
		// Note: will apiVersion >= appInfo.apiVersion? Probably yes, but spec is unclear.
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(physical, &deviceProperties);
		vkGetPhysicalDeviceFeatures(physical, &deviceFeatures);

		uint32_t supportedVersion[] = {
			VK_VERSION_MAJOR(deviceProperties.apiVersion),
			VK_VERSION_MINOR(deviceProperties.apiVersion),
			VK_VERSION_PATCH(deviceProperties.apiVersion)
		};

		std::cout << "physical device supports version " << supportedVersion[0] << "." << supportedVersion[1] << "." << supportedVersion[2] << std::endl;
	}

    void checkSwapChainSupport() {
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount, nullptr);

		if (extensionCount == 0) {
			std::cerr << "physical device doesn't support any extensions" << std::endl;
			exit(1);
		}

		std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physical, nullptr, &extensionCount, deviceExtensions.data());

		for (const auto& extension : deviceExtensions) {
			if (std::string_view(extension.extensionName) ==  std::string_view(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
				std::cout << "physical device supports swap chains" << std::endl;
				return;
			}
		}

		std::cerr << "physical device doesn't support swap chains" << std::endl;
		exit(1);
	}

    void findQueueFamilies() {
		// Check queue families
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, nullptr);

		if (queueFamilyCount == 0) {
			std::cout << "physical device has no queue families!" << std::endl;
			exit(1);
		}

		// Find queue family with graphics support
		// Note: is a transfer queue necessary to copy vertices to the gpu or can a graphics queue handle that?
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, queueFamilies.data());

		std::cout << "physical device has " << queueFamilyCount << " queue families" << std::endl;

		bool foundGraphicsQueueFamily = false;
		bool foundPresentQueueFamily = false;

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			VkBool32 presentSupport = false;
			auto res = vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, windowSurface, &presentSupport);
			if (res != VK_SUCCESS && res != VK_INCOMPLETE) {
				std::cerr << "failed to check surface support for queue family #" << i << std::endl;
				exit(1);
			}

			if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsQueueFamily = i;
				foundGraphicsQueueFamily = true;

				if (presentSupport) {
					presentQueueFamily = i;
					foundPresentQueueFamily = true;
					break;
				}
			}

			if (!foundPresentQueueFamily && presentSupport) {
				presentQueueFamily = i;
				foundPresentQueueFamily = true;
			}
		}

		if (foundGraphicsQueueFamily) {
			std::cout << "queue family #" << graphicsQueueFamily << " supports graphics" << std::endl;

			if (foundPresentQueueFamily) {
				std::cout << "queue family #" << presentQueueFamily << " supports presentation" << std::endl;
			} else {
				std::cerr << "could not find a valid queue family with present support" << std::endl;
				exit(1);
			}
		} else {
			std::cerr << "could not find a valid queue family with graphics support" << std::endl;
			exit(1);
		}
	}
};