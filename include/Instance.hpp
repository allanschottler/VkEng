#pragma once
#include <vector>
#include <iostream>

struct Instance
{
    VkInstance instance;

    Instance() : instance(VK_NULL_HANDLE)
    {
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
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS) {
			std::cerr << "vkCreateInstance failed with error code: " << result << std::endl;
			exit(1);
		}

		std::cout << "Vulkan instance created!\n";
    }

    ~Instance()
    {
        vkDestroyInstance(instance, nullptr);
        std::cout << "destroyed instance" << std::endl;
    }
};