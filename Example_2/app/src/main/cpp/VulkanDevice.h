#ifndef EXAMPLE_2_VULKANDEVICE_H
#define EXAMPLE_2_VULKANDEVICE_H

#include <vector>
#include "VulkanCodeWrapper/vulkan_wrapper.h"


struct VulkanDevice {
public:
    VkInstance vulkanInstance;

    VulkanDevice();
    ~VulkanDevice();
    void createVulkanInstance();
    void createSwapchain();

private:
    // Получаем все доступные слои валидации устройства
    std::vector<VkLayerProperties> getAllValidationLayers();
    // Получаем доступные слои валидации устройства
    std::vector<const char*> getPossibleDebugValidationLayers();
};


#endif
