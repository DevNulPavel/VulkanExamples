#ifndef VULKAN_DESCRIPTOR_POOL_H
#define VULKAN_DESCRIPTOR_POOL_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"


struct VulkanDescriptorPool {
public:
    VulkanDescriptorPool(VulkanLogicalDevicePtr logicalDevice, const std::vector<VkDescriptorPoolSize>& poolSize);
    ~VulkanDescriptorPool();
    VkDescriptorPool getPool() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    std::vector<VkDescriptorPoolSize> _poolSize;
    VkDescriptorPool _pool;
    
private:
};

typedef std::shared_ptr<VulkanDescriptorPool> VulkanDescriptorPoolPtr;

#endif
