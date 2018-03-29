#ifndef VULKAN_SHADER_MODULE_H
#define VULKAN_SHADER_MODULE_H

#include <memory>
#include <vector>
#include <string>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"


struct VulkanShaderModule {
public:
    VulkanShaderModule(VulkanLogicalDevicePtr device, const std::vector<unsigned char>& code);
    ~VulkanShaderModule();
    VkShaderModule getModule() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VkShaderModule _module;
    
private:
};

typedef std::shared_ptr<VulkanShaderModule> VulkanShaderModulePtr;

#endif
