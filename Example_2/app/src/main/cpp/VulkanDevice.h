#ifndef EXAMPLE_2_VULKANDEVICE_H
#define EXAMPLE_2_VULKANDEVICE_H

#include <vector>
#include <map>
#include <string>
#include "VulkanCodeWrapper/vulkan_wrapper.h"


struct VulkanDevice {
public:
    VkInstance vulkanInstance;
    VkDebugReportCallbackEXT vulkanDebugCallback;

public:
    VulkanDevice();
    ~VulkanDevice();
    void createVulkanInstance();    // Создание инстанса
    void setupDebugCallback();      // Устанавливаем коллбек для отладки

private:
    // Получаем все доступные слои валидации устройства
    std::vector<VkLayerProperties> getAllValidationLayers();
    // Список необходимых расширений инстанса приложения
    std::map<std::string, std::vector<VkExtensionProperties>> getAllExtentionsNames(const std::vector<const char *>& layersNames);
    // Список всех расширений в леерах
    void printAllExtentionsAtLayers(const std::vector<const char *>& layersNames);
    // Список необходимых расширений инстанса приложения
    std::vector<const char*> getRequiredExtentionNames();
    // Получаем доступные слои валидации устройства
    std::vector<const char*> getPossibleDebugValidationLayers();
    // К методам расширениям может не быть прямого доступа, поэтому создаем коллбек вручную
    VkResult createDebugReportCallbackEXT(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugReportCallbackEXT* pCallback);
    // Очистка коллбека
    void destroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback,
                                       const VkAllocationCallbacks* pAllocator);
};


#endif
