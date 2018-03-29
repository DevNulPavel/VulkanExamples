#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanCommandPool.h"


struct VulkanCommandBuffer {
public:
    VulkanCommandBuffer(VulkanLogicalDevicePtr logicalDevice, VulkanCommandPoolPtr pool);
    ~VulkanCommandBuffer();
    void begin(VkCommandBufferUsageFlags usageFlags);
    void end();
    void reset(VkCommandBufferResetFlags flags);
    VkCommandBuffer getBuffer() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanCommandPoolPtr getBasePool() const;
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    VulkanCommandPoolPtr _pool;
    VkCommandBuffer _commandBuffer;
    
private:
};

typedef std::shared_ptr<VulkanCommandBuffer> VulkanCommandBufferPtr;

#endif
