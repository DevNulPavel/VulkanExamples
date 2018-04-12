#ifndef VULKAN_COMMAND_POOL_H
#define VULKAN_COMMAND_POOL_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"


class VulkanCommandPool {
public:
    VulkanCommandPool(VulkanLogicalDevicePtr logicalDevice, uint32_t queuesFamilyIndex);
    ~VulkanCommandPool();
    VkCommandPool getPool() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    uint32_t getBaseQueuesFamilyIndex() const;
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    uint32_t _queuesFamilyIndex;
    VkCommandPool _pool;
    
private:
};

typedef std::shared_ptr<VulkanCommandPool> VulkanCommandPoolPtr;

#endif
