#ifndef EXAMPLE_2_VULKANDEVICE_H
#define EXAMPLE_2_VULKANDEVICE_H

#include <vector>
#include "VulkanCodeWrapper/vulkan_wrapper.h"


struct VulkanDevice {
public:
    VkInstance vulkanInstance;

public:
    VulkanDevice();
    ~VulkanDevice();
    void createVulkanInstance();    // Создание инстанса
    void setupDebugCallback();      // Устанавливаем коллбек для отладки
    void createSwapchain();

private:
    // Получаем все доступные слои валидации устройства
    std::vector<VkLayerProperties> getAllValidationLayers();
    // Получаем доступные слои валидации устройства
    std::vector<const char*> getPossibleDebugValidationLayers();
};


#endif
