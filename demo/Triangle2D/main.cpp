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
	std::vector<VkCommandBuffer> graphicsCommandBuffers;

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

		vkFreeCommandBuffers(device->logical, device->commandPool, (uint32_t) graphicsCommandBuffers.size(), graphicsCommandBuffers.data());

		graphicsPipeline.reset();
		// vkDestroyPipeline(device->logical, graphicsPipeline, nullptr);
		// vkDestroyRenderPass(device->logical, swapChain->renderPass, nullptr);
		renderPass.reset();
		framebuffers.clear();
		imageViews.clear();

		// vkDestroyDescriptorSetLayout(device->logical, descriptorSetLayout, nullptr);
		
		if (fullClean) {
			vkDestroySemaphore(device->logical, imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(device->logical, renderingFinishedSemaphore, nullptr);

			// vkDestroyCommandPool(device->logical, commandPool, nullptr);

			// Clean up uniform buffer related objects
			// vkDestroyDescriptorPool(device->logical, descriptorPool, nullptr);
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

			vkDestroySurfaceKHR(instance->instance, windowSurface, nullptr);

			// vkDestroyInstance(instance, nullptr);
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
		
		CommandBuffer copyCommandBuffer(*device, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			CmdCopyBuffer(stage_vertices, *vertexBuffer), 
			CmdCopyBuffer(stage_indices, *indexBuffer));

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

	// void createGraphicsPipeline() {
	// 	auto vertexShader = VertexShader(*device, "shaders/Pass.spv");
	// 	auto fragmentShader = FragmentShader(*device, "shaders/SolidColor.spv");

	// 	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShader.pssCreateInfo, fragmentShader.pssCreateInfo };

	// 	// Describe vertex input
	// 	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	// 	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// 	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	// 	vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	// 	vertexInputCreateInfo.vertexAttributeDescriptionCount = 2;
	// 	vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

	// 	// Describe input assembly
	// 	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	// 	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	// 	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// 	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// 	// Describe viewport and scissor
	// 	VkViewport viewport = {};
	// 	viewport.x = 0.0f;
	// 	viewport.y = 0.0f;
	// 	viewport.width = (float) swapChain->extent.width;
	// 	viewport.height = (float) swapChain->extent.height;
	// 	viewport.minDepth = 0.0f;
	// 	viewport.maxDepth = 1.0f;

	// 	VkRect2D scissor = {};
	// 	scissor.offset.x = 0;
	// 	scissor.offset.y = 0;
	// 	scissor.extent.width = swapChain->extent.width;
	// 	scissor.extent.height = swapChain->extent.height;

	// 	// Note: scissor test is always enabled (although dynamic scissor is possible)
	// 	// Number of viewports must match number of scissors
	// 	VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
	// 	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	// 	viewportCreateInfo.viewportCount = 1;
	// 	viewportCreateInfo.pViewports = &viewport;
	// 	viewportCreateInfo.scissorCount = 1;
	// 	viewportCreateInfo.pScissors = &scissor;

	// 	// Describe rasterization
	// 	// Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
	// 	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
	// 	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// 	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	// 	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	// 	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	// 	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	// 	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	// 	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	// 	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	// 	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	// 	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	// 	rasterizationCreateInfo.lineWidth = 1.0f;

	// 	// Describe multisampling
	// 	// Note: using multisampling also requires turning on device features
	// 	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	// 	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	// 	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	// 	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	// 	multisampleCreateInfo.minSampleShading = 1.0f;
	// 	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	// 	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	// 	// Describing color blending
	// 	// Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
	// 	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	// 	colorBlendAttachmentState.blendEnable = VK_FALSE;
	// 	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	// 	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	// 	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	// 	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// 	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// 	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	// 	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	// 	// Note: all attachments must have the same values unless a device feature is enabled
	// 	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
	// 	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// 	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	// 	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	// 	colorBlendCreateInfo.attachmentCount = 1;
	// 	colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
	// 	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	// 	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	// 	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	// 	colorBlendCreateInfo.blendConstants[3] = 0.0f;

	// 	// Describe pipeline layout
	// 	// Note: this describes the mapping between memory and shader resources (descriptor sets)
	// 	// This is for uniform buffers and samplers
	// 	VkDescriptorSetLayoutBinding layoutBinding = {};
	// 	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// 	layoutBinding.descriptorCount = 1;
	// 	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		
	// 	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
	// 	descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	// 	descriptorLayoutCreateInfo.bindingCount = 1;
	// 	descriptorLayoutCreateInfo.pBindings = &layoutBinding;

	// 	if (vkCreateDescriptorSetLayout(device->logical, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
	// 		std::cerr << "failed to create descriptor layout" << std::endl;
	// 		exit(1);
	// 	} else {
	// 		std::cout << "created descriptor layout" << std::endl;
	// 	}

	// 	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	// 	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// 	layoutCreateInfo.setLayoutCount = 1;
	// 	layoutCreateInfo.pSetLayouts = &descriptorSetLayout;

	// 	if (vkCreatePipelineLayout(device->logical, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
	// 		std::cerr << "failed to create pipeline layout" << std::endl;
	// 		exit(1);
	// 	} else {
	// 		std::cout << "created pipeline layout" << std::endl;
	// 	}

	// 	// Create the graphics pipeline
	// 	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	// 	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// 	pipelineCreateInfo.stageCount = 2;
	// 	pipelineCreateInfo.pStages = shaderStages;
	// 	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	// 	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	// 	pipelineCreateInfo.pViewportState = &viewportCreateInfo;
	// 	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	// 	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	// 	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	// 	pipelineCreateInfo.layout = pipelineLayout;
	// 	pipelineCreateInfo.renderPass = renderPass->renderPass;
	// 	pipelineCreateInfo.subpass = 0;
	// 	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	// 	pipelineCreateInfo.basePipelineIndex = -1;

	// 	if (vkCreateGraphicsPipelines(device->logical, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
	// 		std::cerr << "failed to create graphics pipeline" << std::endl;
	// 		exit(1);
	// 	} else {
	// 		std::cout << "created graphics pipeline" << std::endl;
	// 	}

	// 	// No longer necessary
	// 	// vkDestroyShaderModule(device->logical, vertexShaderModule, nullptr);
	// 	// vkDestroyShaderModule(device->logical, fragmentShaderModule, nullptr);
	// }

	// void createDescriptorPool() {
	// 	// This describes how many descriptor sets we'll create from this pool for each type
	// 	VkDescriptorPoolSize typeCount;
	// 	typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// 	typeCount.descriptorCount = 1;

	// 	VkDescriptorPoolCreateInfo createInfo = {};
	// 	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	// 	createInfo.poolSizeCount = 1;
	// 	createInfo.pPoolSizes = &typeCount;
	// 	createInfo.maxSets = 1;
		
	// 	if (vkCreateDescriptorPool(device->logical, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
	// 		std::cerr << "failed to create descriptor pool" << std::endl;
	// 		exit(1);
	// 	} else {
	// 		std::cout << "created descriptor pool" << std::endl;
	// 	}
	// }

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
		// Allocate graphics command buffers
		graphicsCommandBuffers.resize(swapChain->images.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = device->commandPool;
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
			vkCmdBindDescriptorSets(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			std::cout << "bound descriptor set for command buffer #" << i << std::endl;
			vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipeline);
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

