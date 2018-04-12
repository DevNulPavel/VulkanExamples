#ifndef VULKAN_LOGICAL_DEVICE_H
#define VULKAN_LOGICAL_DEVICE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanQueuesFamiliesIndexes.h"
#include "VulkanSwapChainSupportDetails.h"
#include "VulkanPhysicalDevice.h"

class VulkanQueue;

class VulkanLogicalDevice: public std::enable_shared_from_this<VulkanLogicalDevice> {
public:
    VulkanLogicalDevice(VulkanPhysicalDevicePtr physicalDevice,
                        VulkanQueuesFamiliesIndexes queuesFamiliesIndexes,
                        float presetQueuePriority,
                        uint8_t renderQueuesCount,
                        const std::vector<float>& renderQueuesPriorities,
                        std::vector<const char*> validationLayers,
                        std::vector<const char*> extensions,
                        VkPhysicalDeviceFeatures deviceFeatures);
    ~VulkanLogicalDevice();
    void wait();
    VulkanPhysicalDevicePtr getBasePhysicalDevice() const;
    VulkanQueuesFamiliesIndexes getBaseQueuesFamiliesIndexes() const;
    std::vector<const char*> getBaseValidationLayers() const;
    std::vector<const char*> getBaseExtensions() const;
    VkPhysicalDeviceFeatures getBaseFeatures() const;
    VkDevice getDevice();
    std::vector<std::shared_ptr<VulkanQueue>> getRenderQueues();
    std::shared_ptr<VulkanQueue> getPresentQueue();
    
private:
    VulkanPhysicalDevicePtr _physicalDevice;
    VulkanQueuesFamiliesIndexes _queuesFamiliesIndexes;
    float _presetQueuePriority;
    uint8_t _renderQueuesCount;
    std::vector<float> _renderQueuesPriorities;
    std::vector<const char*> _validationLayers;
    std::vector<const char*> _extensions;
    VkPhysicalDeviceFeatures _deviceFeatures;
    VkDevice _device;
    std::vector<std::shared_ptr<VulkanQueue>> _renderQueues;
    std::shared_ptr<VulkanQueue> _presentQueue;
    
private:
    // Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
    void createLogicalDeviceAndQueue();
};

typedef std::shared_ptr<VulkanLogicalDevice> VulkanLogicalDevicePtr;

#endif
