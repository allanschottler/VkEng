#pragma once
#include <vector>

struct SwapChain {
    Device& device;
    VkExtent2D extent;
	VkFormat format;
	VkSwapchainKHR old;
	VkSwapchainKHR current;
	std::vector<VkImage> images;
	// std::vector<VkImageView> imageViews;
	// std::vector<VkFramebuffer> framebuffers;
    // VkRenderPass renderPass;

    SwapChain(Device& device, VkSurfaceKHR surface, uint32_t width, uint32_t height) 
        : device(device), extent{width, height}, format(VK_FORMAT_UNDEFINED), old(VK_NULL_HANDLE), current(VK_NULL_HANDLE)
    {
        swap();
    }

    ~SwapChain() {
        vkDestroySwapchainKHR(device.logical, current, nullptr);
    }

    void swap() {
		std::cout << "creating swap chain" << std::endl;
        // Find surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical, device.windowSurface, &surfaceCapabilities) != VK_SUCCESS) {
			std::cerr << "failed to acquire presentation surface capabilities" << std::endl;
			exit(1);
		}

		// Find supported surface formats
		uint32_t formatCount;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.physical, device.windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
			std::cerr << "failed to get number of supported surface formats" << std::endl;
			exit(1);
		}

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.physical, device.windowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS) {
			std::cerr << "failed to get supported surface formats" << std::endl;
			exit(1);
		}

		// Find supported present modes
		uint32_t presentModeCount;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(device.physical, device.windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
			std::cerr << "failed to get number of supported presentation modes" << std::endl;
			exit(1);
		}

		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(device.physical, device.windowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
			std::cerr << "failed to get supported presentation modes" << std::endl;
			exit(1);
		}

		// Determine number of images for swap chain
		uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
			imageCount = surfaceCapabilities.maxImageCount;
		}

		std::cout << "using " << imageCount << " images for swap chain" << std::endl;

		// Select a surface format
		VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(surfaceFormats);

		// Select swap chain size
		extent = chooseSwapExtent(surfaceCapabilities);

		// Determine transformation to use (preferring no transform)
		VkSurfaceTransformFlagBitsKHR surfaceTransform;
		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		} else {
			surfaceTransform = surfaceCapabilities.currentTransform;
		}

		// Choose presentation mode (preferring MAILBOX ~= triple buffering)
		VkPresentModeKHR presentMode = choosePresentMode(presentModes);

		// Finally, create the swap chain
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = device.windowSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = surfaceTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = old;

		if (vkCreateSwapchainKHR(device.logical, &createInfo, nullptr, &current) != VK_SUCCESS) {
			std::cerr << "failed to create swap chain" << std::endl;
			exit(1);
		} else {
			std::cout << "created swap chain" << std::endl;
		}

		if (old != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(device.logical, old, nullptr);
		}
		old = current;
		format = surfaceFormat.format;

        // Store the images used by the swap chain
		// Note: these are the images that swap chain image indices refer to
		// Note: actual number of images may differ from requested number, since it's a lower bound
		uint32_t actualImageCount = 0;
		if (vkGetSwapchainImagesKHR(device.logical, current, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0) {
			std::cerr << "failed to acquire number of swap chain images" << std::endl;
			exit(1);
		}

		images.resize(actualImageCount);

		if (vkGetSwapchainImagesKHR(device.logical, current, &actualImageCount, images.data()) != VK_SUCCESS) {
			std::cerr << "failed to acquire swap chain images" << std::endl;
			exit(1);
		}

		std::cout << "acquired swap chain images" << std::endl;
    }

private:
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		// We can either choose any format
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
			return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };

		// Or go with the standard format - if available
		for (const auto& availableSurfaceFormat : availableFormats)
			if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
				return availableSurfaceFormat;
		// Or fall back to the first available one
		return availableFormats[0];
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
		if (surfaceCapabilities.currentExtent.width == uint32_t(-1)) {
			VkExtent2D swapChainExtent = {};

			swapChainExtent.width = std::min(std::max(extent.width, surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
			swapChainExtent.height = std::min(std::max(extent.height, surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);

			return swapChainExtent;
		}
		return surfaceCapabilities.currentExtent;
	}

	VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> presentModes) {
		for (const auto& presentMode : presentModes)
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return presentMode;
		// If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
		return VK_PRESENT_MODE_FIFO_KHR;
	}
};