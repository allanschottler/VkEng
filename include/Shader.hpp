#pragma once
#include <fstream>
#include <iostream>

struct Shader
{
    Device& device;
    VkShaderModule shaderModule;
    VkPipelineShaderStageCreateInfo pssCreateInfo{};

    Shader(Device& device, std::string_view filePath)
        : device(device), shaderModule(VK_NULL_HANDLE)
    {
        std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Failed to open shader file: " << filePath << std::endl;
			std::exit(-1);
		}
		std::vector<char> fileBytes(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(fileBytes.data(), fileBytes.size());
		file.close();

		VkShaderModuleCreateInfo smCreateInfo = {};
		smCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		smCreateInfo.codeSize = fileBytes.size();
		smCreateInfo.pCode = (uint32_t*) fileBytes.data();

		if (vkCreateShaderModule(device.logical, &smCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			std::cerr << "failed to create shader module for " << filePath << std::endl;
			exit(1);
		}

		std::cout << "created shader module for " << filePath << std::endl;
        
        pssCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pssCreateInfo.module = shaderModule;
        pssCreateInfo.pName = "main";        
    }

    ~Shader()
    {
        vkDestroyShaderModule(device.logical, shaderModule, nullptr);
    }
};

struct VertexShader : Shader
{
    VertexShader(Device& device, std::string_view filePath)
        : Shader(device, filePath) 
    {
		pssCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    }
};

struct FragmentShader : Shader
{
    FragmentShader(Device& device, std::string_view filePath)
        : Shader(device, filePath) 
    {
        pssCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
};