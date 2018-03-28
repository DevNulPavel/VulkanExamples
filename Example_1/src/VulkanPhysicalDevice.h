#ifndef VULKAN_PHYSICAL_DEVICE_H
#define VULKAN_PHYSICAL_DEVICE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanQueuesFamiliesIndexes.h"
#include "VulkanSwapChainSupportDetails.h"



struct VulkanPhysicalDevice {
public:
    VkPhysicalDevice physicalDevice;
    
public:
    VulkanPhysicalDevice();
    ~VulkanPhysicalDevice();
    VulkanQueuesFamiliesIndexes getQueuesFamiliesIndexes();
    VulkanSwapChainSupportDetails getSwapChainSupportDetails();

private:
    VulkanQueuesFamiliesIndexes _queuesFamiliesIndexes;
    VulkanSwapChainSupportDetails _swapchainSuppportDetails;
    
private:
    // Дергаем видеокарту
    void pickPhysicalDevice();
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
