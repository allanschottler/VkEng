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
	glm::vec3 pos;
	glm::vec3 color;
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

	// VkInstance instance;
	std::unique_ptr<Instance> instance;
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
	// VkDescriptorSetLayout descriptorSetLayout;
	// VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	// VkExtent2D swapChainExtent;
	// VkFormat swapChainFormat;
	// VkSwapchainKHR oldSwapChain;
	// VkSwapchainKHR swapChain;
	// std::vector<VkImage> swapChainImages;
	// std::vector<VkImageView> swapChainImageViews;
	// std::vector<VkFramebuffer> swapChainFramebuffers;
	// VkRenderPass renderPass;
	std::unique_ptr<SwapChain> swapChain;
	std::vector<ImageView> imageViews;
	std::vector<Framebuffer> framebuffers;
	std::unique_ptr<RenderPass> renderPass;

	// VkPipeline graphicsPipeline;
	// VkPipelineLayout pipelineLayout;
	std::unique_ptr<GraphicsPipeline> graphicsPipeline;

	// VkCommandPool commandPool;
	// std::vector<VkCommandBuffer> graphicsCommandBuffers;
	std::vector<std::unique_ptr<CommandBufferBase>> graphicsCommandBuffers;

	// uint32_t graphicsQueueFamily;
	// uint32_t presentQueueFamily;

	std::chrono::high_resolution_clock::time_point timeStart;

	void setupVulkan() {
		// oldSwapChain = VK_NULL_HANDLE;

		instance = std::make_unique<Instance>();
		// createDebugCallback();
		createWindowSurface();
		// findPhysicalDevice();
		// checkSwapChainSupport();
		// findQueueFamilies();
		// createLogicalDevice();
		device = std::make_unique<Device>(*instance, windowSurface);
		createSemaphores();
		// createCommandPool();
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
		auto vertexShader = VertexShader(*device, "shaders/Pass.spv");
		auto fragmentShader = FragmentShader(*device, "shaders/SolidColor.spv");
		graphicsPipeline = std::make_unique<GraphicsPipeline>(*device, *swapChain, *renderPass, 
			std::move(vertexShader), std::move(fragmentShader), *uniformBuffer, 
			vertexBindingDescription, vertexAttributeDescriptions);
		// createGraphicsPipeline();
		// createDescriptorPool();
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

		swapChain->swap();
		renderPass = std::make_unique<RenderPass>(*device, *swapChain);
		for(auto& image : swapChain->images)
			imageViews.emplace_back(*device, *swapChain, image);
		for(auto& imageView : imageViews)
			framebuffers.emplace_back(*device, *swapChain, imageView, *renderPass);
		
		auto vertexShader = VertexShader(*device, "shaders/Pass.spv");
		auto fragmentShader = FragmentShader(*device, "shaders/SolidColor.spv");
		graphicsPipeline = std::make_unique<GraphicsPipeline>(*device, *swapChain, *renderPass, 
			std::move(vertexShader), std::move(fragmentShader), *uniformBuffer, 
			vertexBindingDescription, vertexAttributeDescriptions);
		createCommandBuffers();
	}

	void cleanup(bool fullClean) {
		vkDeviceWaitIdle(device->logical);

		graphicsCommandBuffers.clear();
		graphicsPipeline.reset();
		renderPass.reset();
		framebuffers.clear();
		imageViews.clear();
		
		if (fullClean) {
			vkDestroySemaphore(device->logical, imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(device->logical, renderingFinishedSemaphore, nullptr);

			uniformBuffer.reset();
			vertexBuffer.reset();
			indexBuffer.reset();
			swapChain.reset();
			device.reset();
			vkDestroySurfaceKHR(instance->instance, windowSurface, nullptr);
			instance.reset();
		}
	}

	void createWindowSurface() {
		if (glfwCreateWindowSurface(instance->instance, window, NULL, &windowSurface) != VK_SUCCESS) {
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
	
	void createVertexBuffer() {
		// Setup vertices
		std::vector<Vertex> vertices = {
			{ { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		// Setup indices
		std::vector<uint32_t> indices = { 0, 1, 2 };
		
		auto stage_vertices = StagingBuffer<Vertex>(*device, vertices);
		vertexBuffer = std::make_unique<DeviceBuffer<Vertex>>(*device, vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		auto stage_indices = StagingBuffer<uint32_t>(*device, indices);
		indexBuffer = std::make_unique<DeviceBuffer<uint32_t>>(*device, indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		
		auto copyCommandBuffer = CommandBuffer(*device,
			CmdCopyBuffer(stage_vertices, *vertexBuffer), 
			CmdCopyBuffer(stage_indices, *indexBuffer));
		copyCommandBuffer.use(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// Submit to queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCommandBuffer.commandBuffer;

		vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->graphicsQueue);

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

	void createDescriptorSet() {
		// There needs to be one descriptor set per binding point in the shader
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = device->descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &graphicsPipeline->descriptorSetLayout;

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
		graphicsCommandBuffers.resize(swapChain->images.size());

		VkClearValue clearColor = {
			{ 0.1f, 0.1f, 0.1f, 1.0f } // R, G, B, A
		};

		// Record command buffer for each swap image
		for (size_t i = 0; i < swapChain->images.size(); i++) {
			graphicsCommandBuffers[i].reset(new CommandBuffer(*device,
				CmdPresent2GraphicsBarrier(*device, *swapChain, swapChain->images[i]),
				CmdBeginRenderPass(*swapChain, *renderPass, framebuffers[i], clearColor),
				CmdBindDescriptorSets(*graphicsPipeline, descriptorSet),
				CmdBindPipeline(*graphicsPipeline),
				CmdBindVertexBuffer(*vertexBuffer),
				CmdBindIndexBuffer(*indexBuffer),
				CmdDrawIndexed(3, 1, 0, 0, 0),
				CmdEndRenderPass(),
				CmdGraphics2PresentBarrier(*device, *swapChain, swapChain->images[i]))
			);
		}
		for (size_t i = 0; i < graphicsCommandBuffers.size(); i++) {
			graphicsCommandBuffers[i]->use(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
			std::cout << "recorded command buffer #" << i << std::endl;
		}

		std::cout << "recorded command buffers" << std::endl;

		// No longer needed
		vkDestroyPipelineLayout(device->logical, graphicsPipeline->pipelineLayout, nullptr);
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
		submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex]->commandBuffer;

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

