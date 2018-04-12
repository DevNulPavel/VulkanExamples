#ifndef VULKAN_FENCE_H
#define VULKAN_FENCE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"


class VulkanFence {
public:
    VulkanFence(VulkanLogicalDevicePtr device, bool signaled);
    ~VulkanFence();
    void waitAndReset();
    VkFence getFence() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VkFence _fence;
    
private:
};

typedef std::shared_ptr<VulkanFence> VulkanFencePtr;

#endif
