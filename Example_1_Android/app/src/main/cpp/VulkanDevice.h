#ifndef EXAMPLE_2_VULKANDEVICE_H
#define EXAMPLE_2_VULKANDEVICE_H

#include <vector>
#include <map>
#include <string>
#include <vulkan_wrapper.h>


////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FamiliesQueueIndexes {
    int32_t renderQueueFamilyIndex;         // Индекс семейства очередей отрисовки
    int32_t renderQueueFamilyQueuesCount;   // Количество очередей в семействе
    int32_t presentQueueFamilyIndex;        // Индекс семейства очередей отображения
    int32_t presentQueueFamilyQueuesCount;  // Количество очередей в семействе

    FamiliesQueueIndexes(){
        renderQueueFamilyIndex = -1;
        renderQueueFamilyQueuesCount = 0;
        presentQueueFamilyIndex = -1;
        presentQueueFamilyQueuesCount = 0;
    }
    bool isComplete(){
        return (renderQueueFamilyIndex >= 0) && (presentQueueFamilyIndex >= 0);
    }
};

// Структура с описанием возможностей свопчейна отрисовки
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

struct VulkanDevice {
public:
    uint32_t windowWidth;
    uint32_t windowHeight;
    std::vector<const char*> vulkanValidationLayers;
    std::vector<const char*> vulkanInstanceExtensions;
    VkInstance vulkanInstance;
    VkDebugReportCallbackEXT vulkanDebugCallback;
    VkSurfaceKHR vulkanSurface;
    VkPhysicalDevice vulkanPhysicalDevice;
    FamiliesQueueIndexes vulkanFamiliesQueueIndexes;
    SwapChainSupportDetails vulkanSwapChainSupportDetails;
    VkDevice vulkanLogicalDevice;
    VkQueue vulkanGraphicsQueue;
    VkQueue vulkanPresentQueue;

public:
    VulkanDevice(ANativeWindow* androidNativeWindow, uint32_t windowW, uint32_t windowH);
    ~VulkanDevice();

private:
    // Стадии загрузки
    void createVulkanInstance();                        // Создание инстанса
    void setupDebugCallback();                          // Устанавливаем коллбек для отладки
    void createSurface(ANativeWindow* androidNativeWindow); // Создаем плоскость отрисовки
    void selectPhysicalDevice();                        // Инициализация физического устройства
    void createLogicalDeviceAndQueue();                 // Создаем логическое устройство для выбранного физического устройства + очередь отрисовки

    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Получаем все доступные слои валидации устройства
    std::vector<VkLayerProperties> getAllValidationLayers();
    // Проверяем, что все запрошенные слои нам доступны
    bool checkAllLayersInVectorAvailable(const std::vector<VkLayerProperties>& allLayers, const std::vector<const char*>& testLayers);
    // Получаем доступные слои валидации устройства
    std::vector<const char*> getPossibleDebugValidationLayers();
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
    // Очистка коллбека
    void destroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback,
                                       const VkAllocationCallbacks* pAllocator);
    // Все возможные расширения физического устройства
    std::vector<VkExtensionProperties> getAllPhysicalDeviceExtentions(VkPhysicalDevice device);
    // Проверяем, поддерживает ли девайс цепочку свопинга
    bool checkPhysicalDeviceRequiredExtensionSupport(VkPhysicalDevice device);
    // Запрашиваем поддержку свопачейна изображений на экране
    SwapChainSupportDetails queryPhysicalDeviceSwapChainSupport(VkPhysicalDevice device);
    // Список семейств очередей на конкретном физическом устройстве
    std::vector<VkQueueFamilyProperties> getPhysicalDeviceAllQueueFamilies(VkPhysicalDevice device);
    // Для данного устройства ищем семейства очередей отрисовки
    FamiliesQueueIndexes findPhysicalDeviceQueueFamiliesIndex(VkPhysicalDevice device);
    // Оценка производительности и пригодности конкретной GPU
    int ratePhysicalDeviceScore(VkPhysicalDevice device);
};


#endif
