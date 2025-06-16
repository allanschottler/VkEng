#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "vkeng.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>

// Configuration
const uint32_t WIDTH = 640;
const uint32_t HEIGHT = 480;

const bool ENABLE_DEBUGGING = false;

const char* DEBUG_LAYER = "VK_LAYER_LUNARG_standard_validation";

// Debug callback
VkBool32 debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		std::cerr << "ERROR: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		std::cerr << "WARNING: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	}

	//exit(1);

	return VK_FALSE;
}

// Vertex layout
struct Vertex {
	float pos[3];
	float color[3];
};

bool windowResized = false;

// Note: support swap chain recreation (not only required for resized windows!)
// Note: window resize may not result in Vulkan telling that the swap chain should be recreated, should be handled explicitly!
class TriangleApplication {
public:
	TriangleApplication() {
		timeStart = std::chrono::high_resolution_clock::now();
	}

	void run() {
		// Note: dynamically loading loader may be a better idea to fail gracefully when Vulkan is not supported

		// Create window for Vulkan
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "The spinning triangle that took 1397 lines of code", nullptr, nullptr);

		glfwSetWindowSizeCallback(window, TriangleApplication::onWindowResized);

		// Use Vulkan
		setupVulkan();

		mainLoop();

		cleanup(true);
	}

private:
	GLFWwindow* window;

	VkInstance instance;
	VkSurfaceKHR windowSurface;
	// VkPhysicalDevice physicalDevice;
	std::unique_ptr<Device> device;
	// VkDevice& device = device->logical;
	// VkDebugReportCallbackEXT callback;
	// VkQueue graphicsQueue;
	// VkQueue presentQueue;
	// VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderingFinishedSemaphore;

	// VkBuffer vertexBuffer;
	// VkDeviceMemory vertexBufferMemory;
	// VkBuffer indexBuffer;
	// VkDeviceMemory indexBufferMemory;
	std::unique_ptr<DeviceBuffer<Vertex>> vertexBuffer;
	std::unique_ptr<DeviceBuffer<uint32_t>> indexBuffer;
	VkVertexInputBindingDescription vertexBindingDescription;
	std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

	struct {
		glm::mat4 transformationMatrix;
	} uniformBufferData;
	// VkBuffer uniformBuffer;
	// VkDeviceMemory uniformBufferMemory;
	std::unique_ptr<UniformBuffer<decltype(uniformBufferData)>> uniformBuffer;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	// VkExtent2D swapChainExtent;
	// VkFormat swapChainFormat;
	// VkSwapchainKHR oldSwapChain;
	// VkSwapchainKHR swapChain;
	// std::vector<VkImage> swapChainImages;
	// std::vector<VkImageView> swapChainImageViews;
	// std::vector<VkFramebuffer> swapChainFramebuffers;
	std::unique_ptr<SwapChain> swapChain;
	std::vector<ImageView> imageViews;
	std::vector<Framebuffer> framebuffers;
	std::unique_ptr<RenderPass> renderPass;

	// VkRenderPass renderPass;
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> graphicsCommandBuffers;

	// uint32_t graphicsQueueFamily;
	// uint32_t presentQueueFamily;

	std::chrono::high_resolution_clock::time_point timeStart;

	void setupVulkan() {
		// oldSwapChain = VK_NULL_HANDLE;

		instance = createInstance();
		// createDebugCallback();
		createWindowSurface();
		// findPhysicalDevice();
		// checkSwapChainSupport();
		// findQueueFamilies();
		// createLogicalDevice();
		device = std::make_unique<Device>(instance, windowSurface);
		createSemaphores();
		createCommandPool();
		createVertexBuffer();
		uniformBuffer = std::make_unique<UniformBuffer<decltype(uniformBufferData)>>(*device, uniformBufferData);
		// createUniformBuffer();
		swapChain = std::make_unique<SwapChain>(*device, windowSurface, WIDTH, HEIGHT);
		// createSwapChain();
		renderPass = std::make_unique<RenderPass>(*device, *swapChain);
		// createRenderPass();
		imageViews.reserve(swapChain->images.size());
		for(auto& image : swapChain->images)
			imageViews.emplace_back(*device, *swapChain, image);		
		// createImageViews();
		framebuffers.reserve(imageViews.size());
		for(auto& imageView : imageViews)
			framebuffers.emplace_back(*device, *swapChain, imageView, *renderPass);
		// createFramebuffers();
		createGraphicsPipeline();
		createDescriptorPool();
		createDescriptorSet();
		createCommandBuffers();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			updateUniformData();
			draw();

			glfwPollEvents();
		}
	}

	static void onWindowResized(GLFWwindow* window, int width, int height) {
		windowResized = true;
	}

	void onWindowSizeChanged() {
		windowResized = false;

		// Only recreate objects that are affected by framebuffer size changes
		cleanup(false);

		// swapChain = std::make_unique<SwapChain>(*device, windowSurface, WIDTH, HEIGHT);
		swapChain->swap();
		// createSwapChain();
		renderPass = std::make_unique<RenderPass>(*device, *swapChain);
		// createRenderPass();
		for(auto& image : swapChain->images)
			imageViews.emplace_back(*device, *swapChain, image);		
		// createImageViews();
		for(auto& imageView : imageViews)
			framebuffers.emplace_back(*device, *swapChain, imageView, *renderPass);
		
		// createSwapChain();
		// createRenderPass();
		// createImageViews();
		// createFramebuffers();
		createGraphicsPipeline();
		createCommandBuffers();
	}

	void cleanup(bool fullClean) {
		vkDeviceWaitIdle(device->logical);

		vkFreeCommandBuffers(device->logical, commandPool, (uint32_t) graphicsCommandBuffers.size(), graphicsCommandBuffers.data());

		vkDestroyPipeline(device->logical, graphicsPipeline, nullptr);
		// vkDestroyRenderPass(device->logical, swapChain->renderPass, nullptr);
		renderPass.reset();

		framebuffers.clear();
		imageViews.clear();

		// for (size_t i = 0; i < swapChain->images.size(); i++) {
			// vkDestroyFramebuffer(device->logical, swapChain->framebuffers[i], nullptr);
			// vkDestroyImageView(device->logical, swapChain->imageViews[i], nullptr);
		// }

		vkDestroyDescriptorSetLayout(device->logical, descriptorSetLayout, nullptr);
		
		if (fullClean) {
			vkDestroySemaphore(device->logical, imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(device->logical, renderingFinishedSemaphore, nullptr);

			vkDestroyCommandPool(device->logical, commandPool, nullptr);

			// Clean up uniform buffer related objects
			vkDestroyDescriptorPool(device->logical, descriptorPool, nullptr);
			// vkDestroyBuffer(device->logical, uniformBuffer, nullptr);
			// vkFreeMemory(device->logical, uniformBufferMemory, nullptr);
			uniformBuffer.reset();

			// Buffers must be destroyed after no command buffers are referring to them anymore
			// vkDestroyBuffer(device->logical, vertexBuffer, nullptr);
			// vkFreeMemory(device->logical, vertexBufferMemory, nullptr);
			// vkDestroyBuffer(device->logical, indexBuffer, nullptr);
			// vkFreeMemory(device->logical, indexBufferMemory, nullptr);
			vertexBuffer.reset();
			indexBuffer.reset();

			// Note: implicitly destroys images (in fact, we're not allowed to do that explicitly)
			// vkDestroySwapchainKHR(device->logical, swapChain->current, nullptr);
			swapChain.reset();

			// vkDestroyDevice(device, nullptr);
			device.reset();

			vkDestroySurfaceKHR(instance, windowSurface, nullptr);

			vkDestroyInstance(instance, nullptr);
		}
	}

	VkInstance createInstance() {
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = "Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		std::vector<const char*> extensions = {
			"VK_KHR_surface",
			"VK_KHR_win32_surface"
		};

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;

		std::cout << "Creating instance with extensions:\n";
		for (const auto* ext : extensions) {
			std::cout << "  " << ext << "\n";
		}

		VkInstance instance;
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS) {
			std::cerr << "vkCreateInstance failed with error code: " << result << std::endl;
			exit(1);
		}

		std::cout << "Vulkan instance created!\n";
		return instance;
	}


	void createWindowSurface() {
		if (glfwCreateWindowSurface(instance, window, NULL, &windowSurface) != VK_SUCCESS) {
			std::cerr << "failed to create window surface!" << std::endl;
			exit(1);
		}

		std::cout << "created window surface" << std::endl;
	}

	void createSemaphores() {
		VkSemaphoreCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device->logical, &createInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device->logical, &createInfo, nullptr, &renderingFinishedSemaphore) != VK_SUCCESS) {
			std::cerr << "failed to create semaphores" << std::endl;
			exit(1);
		} else {
			std::cout << "created semaphores" << std::endl;
		}
	}

	void createCommandPool() {
		// Create graphics command pool
		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = device->graphicsQueueFamily;

		if (vkCreateCommandPool(device->logical, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
			std::cerr << "failed to create command queue for graphics queue family" << std::endl;
			exit(1);
		} else {
			std::cout << "created command pool for graphics queue family" << std::endl;
		}
	}
	
	void createVertexBuffer() {
		// Setup vertices
		std::vector<Vertex> vertices = {
			{ { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		// Setup indices
		std::vector<uint32_t> indices = { 0, 1, 2 };

		// Allocate command buffer for copy operation
		VkCommandBufferAllocateInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufInfo.commandPool = commandPool;
		cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufInfo.commandBufferCount = 1;

		VkCommandBuffer copyCommandBuffer;
		vkAllocateCommandBuffers(device->logical, &cmdBufInfo, &copyCommandBuffer);
		std::cout << "allocated command buffer for vertex buffer copy" << std::endl;

		auto stage_vertices = StagingBuffer<Vertex>(*device, vertices);
		std::cout << "created host buffer for vertices" << std::endl;
		vertexBuffer = std::make_unique<DeviceBuffer<Vertex>>(*device, vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		std::cout << "created device buffer for vertices" << std::endl;
		auto stage_indices = StagingBuffer<uint32_t>(*device, indices);
		std::cout << "created host buffer for indices" << std::endl;
		indexBuffer = std::make_unique<DeviceBuffer<uint32_t>>(*device, indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		std::cout << "created device buffer for indices" << std::endl;

		// Now copy data from host visible buffer to gpu only buffer
		VkCommandBufferBeginInfo bufferBeginInfo = {};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(copyCommandBuffer, &bufferBeginInfo);

		VkBufferCopy copyRegion = {};
		copyRegion.size = vertices.size() * sizeof(vertices[0]);
		vkCmdCopyBuffer(copyCommandBuffer, stage_vertices.buffer, vertexBuffer->buffer, 1, &copyRegion);
		copyRegion.size = indices.size() * sizeof(indices[0]);
		vkCmdCopyBuffer(copyCommandBuffer, stage_indices.buffer, indexBuffer->buffer, 1, &copyRegion);

		vkEndCommandBuffer(copyCommandBuffer);

		// Submit to queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCommandBuffer;

		vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->graphicsQueue);

		vkFreeCommandBuffers(device->logical, commandPool, 1, &copyCommandBuffer);

		// vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
		// vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
		// vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
		// vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);

		std::cout << "set up vertex and index buffers" << std::endl;

		// Binding and attribute descriptions
		vertexBindingDescription.binding = 0;
		vertexBindingDescription.stride = sizeof(vertices[0]);
		vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// vec2 position
		vertexAttributeDescriptions.resize(2);
		vertexAttributeDescriptions[0].binding = 0;
		vertexAttributeDescriptions[0].location = 0;
		vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		
		// vec3 color
		vertexAttributeDescriptions[1].binding = 0;
		vertexAttributeDescriptions[1].location = 1;
		vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributeDescriptions[1].offset = sizeof(float) * 3;
	}

	// void createUniformBuffer() {
	// 	uniformBuffer = std::make_unique<UniformBuffer<decltype(uniformBufferData)>>(*device, uniformBufferData);
	// 	updateUniformData();
	// }

	void updateUniformData() {
		auto timeNow = std::chrono::high_resolution_clock::now();
		long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeStart - timeNow).count();
		float angle = (millis % 4000) / 4000.0f * glm::radians(360.f);

		glm::mat4 modelMatrix{1};
		modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 0, 1));
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f / 3.0f, -0.5f / 3.0f, 0.0f));
		auto viewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
		auto projMatrix = glm::perspective(glm::radians(70.f), swapChain->extent.width / (float) swapChain->extent.height, 0.1f, 10.0f);

		uniformBufferData.transformationMatrix = projMatrix * viewMatrix * modelMatrix;
		uniformBuffer->map(uniformBufferData);
	}

	// Find device memory that is supported by the requirements (typeBits) and meets the desired properties
	VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex) {
		for (uint32_t i = 0; i < 32; i++) {
			if ((typeBits & 1) == 1) {
				if ((device->memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					*typeIndex = i;
					return true;
				}
			}
			typeBits >>= 1;
		}
		return false;
	}

	// void createSwapChain() {
		// // Find surface capabilities
		// VkSurfaceCapabilitiesKHR surfaceCapabilities;
		// if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical, windowSurface, &surfaceCapabilities) != VK_SUCCESS) {
		// 	std::cerr << "failed to acquire presentation surface capabilities" << std::endl;
		// 	exit(1);
		// }

		// // Find supported surface formats
		// uint32_t formatCount;
		// if (vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical, windowSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0) {
		// 	std::cerr << "failed to get number of supported surface formats" << std::endl;
		// 	exit(1);
		// }

		// std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		// if (vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical, windowSurface, &formatCount, surfaceFormats.data()) != VK_SUCCESS) {
		// 	std::cerr << "failed to get supported surface formats" << std::endl;
		// 	exit(1);
		// }

		// // Find supported present modes
		// uint32_t presentModeCount;
		// if (vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical, windowSurface, &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0) {
		// 	std::cerr << "failed to get number of supported presentation modes" << std::endl;
		// 	exit(1);
		// }

		// std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		// if (vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical, windowSurface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
		// 	std::cerr << "failed to get supported presentation modes" << std::endl;
		// 	exit(1);
		// }

		// // Determine number of images for swap chain
		// uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		// if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount) {
		// 	imageCount = surfaceCapabilities.maxImageCount;
		// }

		// std::cout << "using " << imageCount << " images for swap chain" << std::endl;

		// // Select a surface format
		// VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(surfaceFormats);

		// // Select swap chain size
		// swapChainExtent = chooseSwapExtent(surfaceCapabilities);

		// // Determine transformation to use (preferring no transform)
		// VkSurfaceTransformFlagBitsKHR surfaceTransform;
		// if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		// 	surfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		// } else {
		// 	surfaceTransform = surfaceCapabilities.currentTransform;
		// }

		// // Choose presentation mode (preferring MAILBOX ~= triple buffering)
		// VkPresentModeKHR presentMode = choosePresentMode(presentModes);

		// // Finally, create the swap chain
		// VkSwapchainCreateInfoKHR createInfo = {};
		// createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		// createInfo.surface = windowSurface;
		// createInfo.minImageCount = imageCount;
		// createInfo.imageFormat = surfaceFormat.format;
		// createInfo.imageColorSpace = surfaceFormat.colorSpace;
		// createInfo.imageExtent = swapChainExtent;
		// createInfo.imageArrayLayers = 1;
		// createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		// createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// createInfo.queueFamilyIndexCount = 0;
		// createInfo.pQueueFamilyIndices = nullptr;
		// createInfo.preTransform = surfaceTransform;
		// createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// createInfo.presentMode = presentMode;
		// createInfo.clipped = VK_TRUE;
		// createInfo.oldSwapchain = oldSwapChain;

		// if (vkCreateSwapchainKHR(device->logical, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		// 	std::cerr << "failed to create swap chain" << std::endl;
		// 	exit(1);
		// } else {
		// 	std::cout << "created swap chain" << std::endl;
		// }

		// if (oldSwapChain != VK_NULL_HANDLE) {
		// 	vkDestroySwapchainKHR(device->logical, oldSwapChain, nullptr);
		// }
		// oldSwapChain = swapChain;

		// swapChainFormat = surfaceFormat.format;

		// // Store the images used by the swap chain
		// // Note: these are the images that swap chain image indices refer to
		// // Note: actual number of images may differ from requested number, since it's a lower bound
		// uint32_t actualImageCount = 0;
		// if (vkGetSwapchainImagesKHR(device->logical, swapChain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0) {
		// 	std::cerr << "failed to acquire number of swap chain images" << std::endl;
		// 	exit(1);
		// }

		// swapChainImages.resize(actualImageCount);

		// if (vkGetSwapchainImagesKHR(device->logical, swapChain, &actualImageCount, swapChainImages.data()) != VK_SUCCESS) {
		// 	std::cerr << "failed to acquire swap chain images" << std::endl;
		// 	exit(1);
		// }

		// std::cout << "acquired swap chain images" << std::endl;
	// }

	// VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// 	// We can either choose any format
	// 	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
	// 		return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	// 	}

	// 	// Or go with the standard format - if available
	// 	for (const auto& availableSurfaceFormat : availableFormats) {
	// 		if (availableSurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
	// 			return availableSurfaceFormat;
	// 		}
	// 	}

	// 	// Or fall back to the first available one
	// 	return availableFormats[0];
	// }

	// VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
	// 	if (surfaceCapabilities.currentExtent.width == uint32_t(-1)) {
	// 		VkExtent2D swapChainExtent = {};

	// 		swapChainExtent.width = std::min(std::max(WIDTH, surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
	// 		swapChainExtent.height = std::min(std::max(HEIGHT, surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);

	// 		return swapChainExtent;
	// 	} else {
	// 		return surfaceCapabilities.currentExtent;
	// 	}
	// }

	// VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> presentModes) {
	// 	for (const auto& presentMode : presentModes) {
	// 		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
	// 			return presentMode;
	// 		}
	// 	}

	// 	// If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
	// 	return VK_PRESENT_MODE_FIFO_KHR;
	// }

	// void createRenderPass() {
	// 	VkAttachmentDescription attachmentDescription = {};
	// 	attachmentDescription.format = swapChainFormat;
	// 	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	// 	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// 	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// 	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// 	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// 	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// 	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// 	// Note: hardware will automatically transition attachment to the specified layout
	// 	// Note: index refers to attachment descriptions array
	// 	VkAttachmentReference colorAttachmentReference = {};
	// 	colorAttachmentReference.attachment = 0;
	// 	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// 	// Note: this is a description of how the attachments of the render pass will be used in this sub pass
	// 	// e.g. if they will be read in shaders and/or drawn to
	// 	VkSubpassDescription subPassDescription = {};
	// 	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	// 	subPassDescription.colorAttachmentCount = 1;
	// 	subPassDescription.pColorAttachments = &colorAttachmentReference;

	// 	// Create the render pass
	// 	VkRenderPassCreateInfo createInfo = {};
	// 	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	// 	createInfo.attachmentCount = 1;
	// 	createInfo.pAttachments = &attachmentDescription;
	// 	createInfo.subpassCount = 1;
	// 	createInfo.pSubpasses = &subPassDescription;

	// 	if (vkCreateRenderPass(device->logical, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
	// 		std::cerr << "failed to create render pass" << std::endl;
	// 		exit(1);
	// 	} else {
	// 		std::cout << "created render pass" << std::endl;
	// 	}
	// }

	// void createImageViews() {
	// 	swapChainImageViews.resize(swapChainImages.size());

	// 	// Create an image view for every image in the swap chain
	// 	for (size_t i = 0; i < swapChainImages.size(); i++) {
	// 		VkImageViewCreateInfo createInfo = {};
	// 		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	// 		createInfo.image = swapChainImages[i];
	// 		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	// 		createInfo.format = swapChainFormat;
	// 		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	// 		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	// 		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	// 		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	// 		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// 		createInfo.subresourceRange.baseMipLevel = 0;
	// 		createInfo.subresourceRange.levelCount = 1;
	// 		createInfo.subresourceRange.baseArrayLayer = 0;
	// 		createInfo.subresourceRange.layerCount = 1;

	// 		if (vkCreateImageView(device->logical, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
	// 			std::cerr << "failed to create image view for swap chain image #" << i << std::endl;
	// 			exit(1);
	// 		}
	// 	}

	// 	std::cout << "created image views for swap chain images" << std::endl;
	// }

	// void createFramebuffers() {
	// 	swapChainFramebuffers.resize(swapChainImages.size());

	// 	// Note: Framebuffer is basically a specific choice of attachments for a render pass
	// 	// That means all attachments must have the same dimensions, interesting restriction
	// 	for (size_t i = 0; i < swapChainImages.size(); i++) {
	// 		VkFramebufferCreateInfo createInfo = {};
	// 		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	// 		createInfo.renderPass = renderPass;
	// 		createInfo.attachmentCount = 1;
	// 		createInfo.pAttachments = &swapChainImageViews[i];
	// 		createInfo.width = swapChainExtent.width;
	// 		createInfo.height = swapChainExtent.height;
	// 		createInfo.layers = 1;

	// 		if (vkCreateFramebuffer(device->logical, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
	// 			std::cerr << "failed to create framebuffer for swap chain image view #" << i << std::endl;
	// 			exit(1);
	// 		}
	// 	}

	// 	std::cout << "created framebuffers for swap chain image views" << std::endl;
	// }

	VkShaderModule createShaderModule(const std::string& filename) { 
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Failed to open shader file: " << filename << std::endl;
			std::exit(-1);
		}
		std::vector<char> fileBytes(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(fileBytes.data(), fileBytes.size());
		file.close();

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = fileBytes.size();
		createInfo.pCode = (uint32_t*) fileBytes.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device->logical, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			std::cerr << "failed to create shader module for " << filename << std::endl;
			exit(1);
		}

		std::cout << "created shader module for " << filename << std::endl;

		return shaderModule;
	}

	void createGraphicsPipeline() {
		VkShaderModule vertexShaderModule = createShaderModule("shaders/Pass.spv");
		VkShaderModule fragmentShaderModule = createShaderModule("shaders/SolidColor.spv");

		// Set up shader stage info
		VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
		vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderCreateInfo.module = vertexShaderModule;
		vertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
		fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderCreateInfo.module = fragmentShaderModule;
		fragmentShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

		// Describe vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 2;
		vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

		// Describe input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Describe viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapChain->extent.width;
		viewport.height = (float) swapChain->extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = swapChain->extent.width;
		scissor.extent.height = swapChain->extent.height;

		// Note: scissor test is always enabled (although dynamic scissor is possible)
		// Number of viewports must match number of scissors
		VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.pViewports = &viewport;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.pScissors = &scissor;

		// Describe rasterization
		// Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
		rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationCreateInfo.depthBiasClamp = 0.0f;
		rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationCreateInfo.lineWidth = 1.0f;

		// Describe multisampling
		// Note: using multisampling also requires turning on device features
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.minSampleShading = 1.0f;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

		// Describing color blending
		// Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		// Note: all attachments must have the same values unless a device feature is enabled
		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendCreateInfo.attachmentCount = 1;
		colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendCreateInfo.blendConstants[0] = 0.0f;
		colorBlendCreateInfo.blendConstants[1] = 0.0f;
		colorBlendCreateInfo.blendConstants[2] = 0.0f;
		colorBlendCreateInfo.blendConstants[3] = 0.0f;

		// Describe pipeline layout
		// Note: this describes the mapping between memory and shader resources (descriptor sets)
		// This is for uniform buffers and samplers
		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
		descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCreateInfo.bindingCount = 1;
		descriptorLayoutCreateInfo.pBindings = &layoutBinding;

		if (vkCreateDescriptorSetLayout(device->logical, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			std::cerr << "failed to create descriptor layout" << std::endl;
			exit(1);
		} else {
			std::cout << "created descriptor layout" << std::endl;
		}

		VkPipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.setLayoutCount = 1;
		layoutCreateInfo.pSetLayouts = &descriptorSetLayout;

		if (vkCreatePipelineLayout(device->logical, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			std::cerr << "failed to create pipeline layout" << std::endl;
			exit(1);
		} else {
			std::cout << "created pipeline layout" << std::endl;
		}

		// Create the graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = renderPass->renderPass;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device->logical, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			std::cerr << "failed to create graphics pipeline" << std::endl;
			exit(1);
		} else {
			std::cout << "created graphics pipeline" << std::endl;
		}

		// No longer necessary
		vkDestroyShaderModule(device->logical, vertexShaderModule, nullptr);
		vkDestroyShaderModule(device->logical, fragmentShaderModule, nullptr);
	}

	void createDescriptorPool() {
		// This describes how many descriptor sets we'll create from this pool for each type
		VkDescriptorPoolSize typeCount;
		typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		typeCount.descriptorCount = 1;

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &typeCount;
		createInfo.maxSets = 1;
		
		if (vkCreateDescriptorPool(device->logical, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			std::cerr << "failed to create descriptor pool" << std::endl;
			exit(1);
		} else {
			std::cout << "created descriptor pool" << std::endl;
		}
	}

	void createDescriptorSet() {
		// There needs to be one descriptor set per binding point in the shader
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(device->logical, &allocInfo, &descriptorSet) != VK_SUCCESS) {
			std::cerr << "failed to create descriptor set" << std::endl;
			exit(1);
		} else {
			std::cout << "created descriptor set" << std::endl;
		}

		// Update descriptor set with uniform binding
		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = uniformBuffer->buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(uniformBufferData);

		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
		writeDescriptorSet.dstBinding = 0;

		vkUpdateDescriptorSets(device->logical, 1, &writeDescriptorSet, 0, nullptr);
	}

	void createCommandBuffers() {
		// Allocate graphics command buffers
		graphicsCommandBuffers.resize(swapChain->images.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) swapChain->images.size();

		if (vkAllocateCommandBuffers(device->logical, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS) {
			std::cerr << "failed to allocate graphics command buffers" << std::endl;
			exit(1);
		} else {
			std::cout << "allocated graphics command buffers" << std::endl;
		}

		// Prepare data for recording command buffers
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkImageSubresourceRange subResourceRange = {};
		subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResourceRange.baseMipLevel = 0;
		subResourceRange.levelCount = 1;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.layerCount = 1;

		VkClearValue clearColor = {
			{ 0.1f, 0.1f, 0.1f, 1.0f } // R, G, B, A
		};

		// Record command buffer for each swap image
		for (size_t i = 0; i < swapChain->images.size(); i++) {
			vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);
			std::cout << "recording command buffer #" << i << std::endl;

			// If present queue family and graphics queue family are different, then a barrier is necessary
			// The barrier is also needed initially to transition the image to the present layout
			VkImageMemoryBarrier presentToDrawBarrier = {};
			presentToDrawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			presentToDrawBarrier.srcAccessMask = 0;
			presentToDrawBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			presentToDrawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			presentToDrawBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			if (device->presentQueueFamily != device->graphicsQueueFamily) {
				presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			} else {
				presentToDrawBarrier.srcQueueFamilyIndex = device->presentQueueFamily;
				presentToDrawBarrier.dstQueueFamilyIndex = device->graphicsQueueFamily;
			}

			presentToDrawBarrier.image = swapChain->images[i];
			presentToDrawBarrier.subresourceRange = subResourceRange;

			vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToDrawBarrier);
			std::cout << "transitioned image #" << i << " from present to draw layout" << std::endl;

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass->renderPass;
			renderPassBeginInfo.framebuffer = framebuffers[i].framebuffer;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent = swapChain->extent;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			std::cout << "started render pass for command buffer #" << i << std::endl;
			vkCmdBindDescriptorSets(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			std::cout << "bound descriptor set for command buffer #" << i << std::endl;
			vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			std::cout << "bound graphics pipeline for command buffer #" << i << std::endl;
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(graphicsCommandBuffers[i], 0, 1, &vertexBuffer->buffer, &offset);
			std::cout << "bound vertex buffer for command buffer #" << i << std::endl;
			vkCmdBindIndexBuffer(graphicsCommandBuffers[i], indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			std::cout << "bound index buffer for command buffer #" << i << std::endl;
			vkCmdDrawIndexed(graphicsCommandBuffers[i], 3, 1, 0, 0, 0);
			std::cout << "issued draw command for command buffer #" << i << std::endl;
			vkCmdEndRenderPass(graphicsCommandBuffers[i]);
			std::cout << "ended render pass for command buffer #" << i << std::endl;
			// If present and graphics queue families differ, then another barrier is required
			if (device->presentQueueFamily != device->graphicsQueueFamily) {
				VkImageMemoryBarrier drawToPresentBarrier = {};
				drawToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				drawToPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				drawToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				drawToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				drawToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				drawToPresentBarrier.srcQueueFamilyIndex = device->graphicsQueueFamily;
				drawToPresentBarrier.dstQueueFamilyIndex = device->presentQueueFamily;
				drawToPresentBarrier.image = swapChain->images[i];
				drawToPresentBarrier.subresourceRange = subResourceRange;

				vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &drawToPresentBarrier);
				std::cout << "transitioned image #" << i << " from draw to present layout" << std::endl;
			}

			if (vkEndCommandBuffer(graphicsCommandBuffers[i]) != VK_SUCCESS) {
				std::cerr << "failed to record command buffer" << std::endl;
				exit(1);
			}
			std::cout << "recorded command buffer #" << i << std::endl;
		}

		std::cout << "recorded command buffers" << std::endl;

		// No longer needed
		vkDestroyPipelineLayout(device->logical, pipelineLayout, nullptr);
	}

	void draw() {
		// Acquire image
		uint32_t imageIndex;
		VkResult res = vkAcquireNextImageKHR(device->logical, swapChain->current, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		// Unless surface is out of date right now, defer swap chain recreation until end of this frame
		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			onWindowSizeChanged();
			return;
		} else if (res != VK_SUCCESS) {
			std::cerr << "failed to acquire image" << std::endl;
			exit(1);
		}

		// Wait for image to be available and draw
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

		// This is the stage where the queue should wait on the semaphore
		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex];

		if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			std::cerr << "failed to submit draw command buffer" << std::endl;
			exit(1);
		}

		// Present drawn image
		// Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain->current;
		presentInfo.pImageIndices = &imageIndex;

		res = vkQueuePresentKHR(device->presentQueue, &presentInfo);

		if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || windowResized) {
			onWindowSizeChanged();
		} else if (res != VK_SUCCESS) {
			std::cerr << "failed to submit present command buffer" << std::endl;
			exit(1);
		}
	}
};

int main() {
    std::cout << "main() started" << std::endl;

    TriangleApplication* app = nullptr;

    try {
        std::cout << "allocating app..." << std::endl;
        app = new TriangleApplication(); // likely crash here
        std::cout << "constructed app..." << std::endl;

        app->run();
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }

    delete app;
    std::cout << "main() ended" << std::endl;
    return 0;
}

