#ifndef VULKAN_DESCRIPTOR_SET_H
#define VULKAN_DESCRIPTOR_SET_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"



struct VulkanDescriptorSetUpdateConfig {
    VkDescriptorType type;
    uint32_t binding;
    union{
        VkDescriptorImageInfo imageInfo;
        VkDescriptorBufferInfo bufferInfo;
    };
    
    VulkanDescriptorSetUpdateConfig();
};

struct VulkanDescriptorSet {
public:
    VulkanDescriptorSet(VulkanLogicalDevicePtr logicalDevice, VulkanDescriptorSetLayoutPtr layout, VulkanDescriptorPoolPtr pool);
    ~VulkanDescriptorSet();
    void updateDescriptorSet(const std::vector<VulkanDescriptorSetUpdateConfig>& configs);
    VkDescriptorSet getSet() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanDescriptorSetLayoutPtr getBaseLayout() const;
    VulkanDescriptorPoolPtr getBasePool() const;
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    VulkanDescriptorSetLayoutPtr _layout;
    VulkanDescriptorPoolPtr _pool;
    VkDescriptorSet _set;
    
private:
};

typedef std::shared_ptr<VulkanDescriptorSet> VulkanDescriptorSetPtr;

#endif
