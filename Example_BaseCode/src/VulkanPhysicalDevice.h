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
    VulkanPhysicalDevice(VulkanInstancePtr instance,
						 const std::vector<const char*>& extensions,
						 const std::vector<const char*>& layers,
						 VulkanSurfacePtr surface);
    ~VulkanPhysicalDevice();
    void updateSwapchainSupportDetails();   // Обновляем информацию о свопчейне после ресайза окна
    VkPhysicalDevice getDevice() const;
    const VkPhysicalDeviceFeatures& getPossibleDeviceFeatures() const;
    const VkPhysicalDeviceProperties& getDeviceProperties() const;
    VulkanQueuesFamiliesIndexes getQueuesFamiliesIndexes() const;
    VulkanSwapChainSupportDetails getSwapChainSupportDetails() const;
    VulkanInstancePtr getBaseInstance() const;
    std::vector<const char*> getBaseExtentions() const;
    VulkanSurfacePtr getBaseSurface() const;
    
private:
    VulkanInstancePtr _vulkanInstance;
    std::vector<const char*> _vulkanExtensions;
	std::vector<const char*> _vulkanLayers;
    VulkanSurfacePtr _vulkanSurface;
    VkPhysicalDevice _device;
    VkPhysicalDeviceFeatures _deviceFeatures;
    VkPhysicalDeviceProperties _deviceProperties;
    VulkanQueuesFamiliesIndexes _queuesFamiliesIndexes;
    VulkanSwapChainSupportDetails _swapchainSuppportDetails;
    
private:
    // Проверяем, поддерживает ли девайс цепочку свопинга
    bool checkDeviceRequiredExtensionSupport(VkPhysicalDevice device);
	// Смотрим - есть ли у данного устройства поддержка нужных слоев
	bool checkDeviceRequiredLayerSupport(VkPhysicalDevice device);
    // Оценка производительности и пригодности конкретной GPU
    int rateDeviceScore(VkPhysicalDevice device);
    // Запрашиваем поддержку свопачейна изображений на экране
    VulkanSwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    // Для данного устройства ищем очереди отрисовки
    VulkanQueuesFamiliesIndexes findQueueFamiliesIndexInDevice(VkPhysicalDevice device);
};

typedef std::shared_ptr<VulkanPhysicalDevice> VulkanPhysicalDevicePtr;

#endif
