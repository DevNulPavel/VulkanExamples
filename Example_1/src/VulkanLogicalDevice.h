#ifndef VULKAN_LOGICAL_DEVICE_H
#define VULKAN_LOGICAL_DEVICE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanQueuesFamiliesIndexes.h"
#include "VulkanSwapChainSupportDetails.h"
#include "VulkanPhysicalDevice.h"

struct VulkanQueue;

struct VulkanLogicalDevice: public std::enable_shared_from_this<VulkanLogicalDevice> {
public:
    
    
public:
    VulkanLogicalDevice(VulkanPhysicalDevicePtr physicalDevice, VulkanQueuesFamiliesIndexes queuesFamiliesIndexes, std::vector<const char*> validationLayers, std::vector<const char*> extensions);
    ~VulkanLogicalDevice();
    VulkanPhysicalDevicePtr getBasePhysicalDevice() const;
    VulkanQueuesFamiliesIndexes getBaseQueuesFamiliesIndexes() const;
    std::vector<const char*> getBaseValidationLayers() const;
    std::vector<const char*> getBaseExtensions() const;
    VkDevice getDevice();
    std::shared_ptr<VulkanQueue> getRenderQueue();
    std::shared_ptr<VulkanQueue> getPresentQueue();
    
private:
    VulkanPhysicalDevicePtr _physicalDevice;
    VulkanQueuesFamiliesIndexes _queuesFamiliesIndexes;
    std::vector<const char*> _validationLayers;
    std::vector<const char*> _extensions;
    VkDevice _device;
    std::shared_ptr<VulkanQueue> _renderQueue;
    std::shared_ptr<VulkanQueue> _presentQueue;
    
private:
    // Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
    void createLogicalDeviceAndQueue();
};

typedef std::shared_ptr<VulkanLogicalDevice> VulkanLogicalDevicePtr;

#endif
