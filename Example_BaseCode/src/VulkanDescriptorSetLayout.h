#ifndef VULKAN_DESCRIPTOR_SET_LAYOUT_H
#define VULKAN_DESCRIPTOR_SET_LAYOUT_H

#include <memory>
#include <vector>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"


struct VulkanDescriptorSetConfig{
    uint32_t binding;
    uint32_t desriptorsCount;
    VkDescriptorType desriptorType;
    VkShaderStageFlags descriptorStageFlags;
    
    VulkanDescriptorSetConfig();
};

struct VulkanDescriptorSetLayout {
public:
    VulkanDescriptorSetLayout(VulkanLogicalDevicePtr device, const std::vector<VulkanDescriptorSetConfig>& configs);
    ~VulkanDescriptorSetLayout();
    VkDescriptorSetLayout getLayout() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    std::vector<VulkanDescriptorSetConfig> getConfig() const;

private:
    VulkanLogicalDevicePtr _device;
    std::vector<VulkanDescriptorSetConfig> _config;
    VkDescriptorSetLayout _layout;
    
private:
};

typedef std::shared_ptr<VulkanDescriptorSetLayout> VulkanDescriptorSetLayoutPtr;

#endif
