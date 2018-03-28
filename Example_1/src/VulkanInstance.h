#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include <vector>
#include <map>
#include <string>
#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


struct VulkanInstance {
public:
    std::vector<const char*> validationLayers;
    std::vector<const char*> instanceExtensions;
    VkInstance instance;
    VkDebugReportCallbackEXT debugCallback;

public:
    VulkanInstance();
    ~VulkanInstance();
    
private:
    // Создание инстанса Vulkan
    void createVulkanInstance();
    // Устанавливаем коллбек для отладки
    void setupDebugCallback();
    
    ////////////////////////////////////////////////////////////
    
    // Получаем все доступные слои валидации устройства
    std::vector<VkLayerProperties> getAllValidationLayers();
    // Проверяем, что все запрошенные слои нам доступны
    bool checkAllLayersInVectorAvailable(const std::vector<VkLayerProperties>& allLayers, const std::vector<const char*>& testLayers);
    // Получаем доступные слои валидации устройства
    std::vector<const char *> getPossibleDebugValidationLayers();
    // Список необходимых расширений инстанса приложения
    std::map<std::string, std::vector<VkExtensionProperties>> getAllExtentionsNames(const std::vector<const char *>& layersNames);
    // Список всех расширений в леерах
    void printAllExtentionsAtLayers(const std::vector<const char *>& layersNames);
    // Список необходимых расширений инстанса приложения
    std::vector<const char*> getRequiredInstanceExtentionNames();
    // К методам расширениям может не быть прямого доступа, поэтому создаем коллбек вручную
    VkResult createDebugReportCallbackEXT(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkDebugReportCallbackEXT* pCallback);
    // Убираем коллбек
    void destroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback,
                                       const VkAllocationCallbacks* pAllocator);
};

typedef std::shared_ptr<VulkanInstance> VulkanInstancePtr;

#endif
