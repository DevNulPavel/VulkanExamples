#ifndef VULKAN_PHYSICAL_DEVICE_H
#define VULKAN_PHYSICAL_DEVICE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanQueuesFamiliesIndexes.h"
#include "VulkanSwapChainSupportDetails.h"
#include "VulkanInstance.h"
#include "VulkanSurface.h"



struct VulkanPhysicalDevice {
public:
    VulkanPhysicalDevice(VulkanInstancePtr instance, std::vector<const char*> extensions, VulkanSurfacePtr surface);
    ~VulkanPhysicalDevice();
    VkPhysicalDevice getDevice() const;
    VulkanQueuesFamiliesIndexes getQueuesFamiliesIndexes() const;
    VulkanSwapChainSupportDetails getSwapChainSupportDetails() const;
    VulkanInstancePtr getBaseInstance() const;
    std::vector<const char*> getBaseExtentions() const;
    VulkanSurfacePtr getBaseSurface() const;
    
private:
    VulkanInstancePtr _vulkanInstance;
    std::vector<const char*> _vulkanExtensions;
    VulkanSurfacePtr _vulkanSurface;
    VkPhysicalDevice _device;
    VulkanQueuesFamiliesIndexes _queuesFamiliesIndexes;
    VulkanSwapChainSupportDetails _swapchainSuppportDetails;
    
private:
    // Проверяем, поддерживает ли девайс цепочку свопинга
    bool checkDeviceRequiredExtensionSupport(VkPhysicalDevice device);
    // Оценка производительности и пригодности конкретной GPU
    int rateDeviceScore(VkPhysicalDevice device);
    // Запрашиваем поддержку свопачейна изображений на экране
    VulkanSwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    // Для данного устройства ищем очереди отрисовки
    VulkanQueuesFamiliesIndexes findQueueFamiliesIndexInDevice(VkPhysicalDevice device);
};

typedef std::shared_ptr<VulkanPhysicalDevice> VulkanPhysicalDevicePtr;

#endif
