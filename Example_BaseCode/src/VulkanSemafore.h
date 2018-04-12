#ifndef VULKAN_SEMAFORE_H
#define VULKAN_SEMAFORE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"


class VulkanSemafore {
public:
    VulkanSemafore(VulkanLogicalDevicePtr device);
    ~VulkanSemafore();
    VkSemaphore getSemafore() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VkSemaphore _semafore;
    
private:
};

typedef std::shared_ptr<VulkanSemafore> VulkanSemaforePtr;

#endif
