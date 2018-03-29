#include "VulkanShaderModule.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>


VulkanShaderModule::VulkanShaderModule(VulkanLogicalDevicePtr device, const std::string& code):
    _device(device){
        
    VkShaderModuleCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkShaderModuleCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = static_cast<uint32_t>(code.size());
    createInfo.pCode = (uint32_t*)code.data();
    
    if (vkCreateShaderModule(_device->getDevice(), &createInfo, nullptr, &_module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
}

VulkanShaderModule::~VulkanShaderModule(){
    vkDestroyShaderModule(_device->getDevice(), _module, nullptr);
}

VkShaderModule VulkanShaderModule::getModule() const{
    return _module;
}
