#include "VulkanDevice.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <set>
#include <array>
#include <vector>
#include <android/log.h>
#include <dlfcn.h>  // For dlopen
#include "SupportFunctions.h"


#define VALIDATION_LAYERS_ENABLED

#define  LOG_TAG    "VulkanTest"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


// Отладочный коллбек
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
                                                    VkDebugReportObjectTypeEXT objType,
                                                    uint64_t obj,
                                                    size_t location,
                                                    int32_t code,
                                                    const char* layerPrefix,
                                                    const char* msg,
                                                    void* userData) {
    printf("Validation layer message %s: %s\n", layerPrefix, msg);
    fflush(stdout);
    return VK_FALSE;
}


VulkanDevice::VulkanDevice():
   vulkanInstance(VK_NULL_HANDLE),
   vulkanDebugCallback(VK_NULL_HANDLE){
}

VulkanDevice::~VulkanDevice() {
#ifdef VALIDATION_LAYERS_ENABLED
    destroyDebugReportCallbackEXT(vulkanDebugCallback, nullptr);
#endif
    vkDestroyInstance(vulkanInstance, nullptr);
}

// Получаем все доступные слои валидации устройства
std::vector<VkLayerProperties> VulkanDevice::getAllValidationLayers(){
    // Количество уровней валидации
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Получаем доступные слои валидации
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return availableLayers;
}

// Проверяем, что все запрошенные слои нам доступны
bool checkAllLayersInVectorAvailable(const std::vector<VkLayerProperties>& allLayers, const std::vector<const char*>& testLayers){
    for(int i = 0; i < testLayers.size(); i++) {
        const char* layerName = testLayers[i];
        bool layerFound = false;

        for (const auto& layerProperties : allLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            LOGD("Layer %s not available!\n", layerName);
            fflush(stdout);
            return false;
        }
    }
    return true;
}

// Список необходимых расширений инстанса приложения
std::vector<VkExtensionProperties> VulkanDevice::getAllExtentionsNames(const std::vector<const char *>& layersNames){
    std::vector<VkExtensionProperties> result;

    for (const char* layerName: layersNames) {
        std::vector<VkExtensionProperties> layerExtentions;

        // Количество расширений доступных
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);

        // Получаем расширения
        layerExtentions.resize(extensionCount);
        vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, layerExtentions.data());

        result.insert(result.end(), layerExtentions.begin(), layerExtentions.end());
    }

    return result;
}

// Список необходимых расширений инстанса приложения
std::vector<const char*> VulkanDevice::getRequiredExtentionNames(const std::vector<const char *>& layersNames){
    std::vector<VkExtensionProperties> allExtentions = getAllExtentionsNames(layersNames);
    for(const VkExtensionProperties& extentionInfo: allExtentions){
        LOGD("Extention available: %s\n", extentionInfo.extensionName);
        fflush(stdout);
    }

    std::vector<const char*> result;
    result.push_back("VK_KHR_surface");
    result.push_back("VK_KHR_android_surface");
#ifdef VALIDATION_LAYERS_ENABLED
    result.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
    return result;
}

// Получаем доступные слои валидации устройства
std::vector<const char *> VulkanDevice::getPossibleDebugValidationLayers(){
#ifdef VALIDATION_LAYERS_ENABLED
    // Список всех слоев
    std::vector<VkLayerProperties> allValidationLayers = getAllValidationLayers();
    for(const VkLayerProperties& layerInfo: allValidationLayers){
        LOGD("Validation layer available: %s (%s)\n", layerInfo.layerName, layerInfo.description);
        fflush(stdout);
    }

    // Возможные отладочные слои
    std::vector<const char*> result;
    result.push_back("VK_LAYER_LUNARG_standard_validation");
    if (!checkAllLayersInVectorAvailable(allValidationLayers, result)) {
        result.clear();
        result.push_back("VK_LAYER_LUNARG_image");
        result.push_back("VK_LAYER_GOOGLE_threading");
        result.push_back("VK_LAYER_LUNARG_parameter_validation");
        result.push_back("VK_LAYER_LUNARG_object_tracker");
        result.push_back("VK_LAYER_LUNARG_core_validation");
        result.push_back("VK_LAYER_GOOGLE_unique_objects");
        result.push_back("VK_LAYER_LUNARG_swapchain");

        if (!checkAllLayersInVectorAvailable(allValidationLayers, result)) {
            LOGE("Failed to get validation layers!\n");
            fflush(stdout);
            throw std::runtime_error("Failed to create instance!");
        }
    }

    return result;
#else
    return std::vector<const char*>();
#endif
}

void VulkanDevice::createVulkanInstance(){

    // Структура с настройками приложения Vulkan
    VkApplicationInfo appInfo = {};
    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;  // Указываем используемую версию Vulkan

    // Запрашиваем возможные слои валидации
    std::vector<const char*> validationLayers = getPossibleDebugValidationLayers();
    std::vector<const char*> instanceExtensions = getRequiredExtentionNames(validationLayers);

    // Структура настроек создания инстанса
    VkInstanceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());  // Включаем расширения
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());     // Включаем стандартные слои валидации
    createInfo.ppEnabledLayerNames = validationLayers.data();

    // Непосредственно создание инстанса Vulkan
    VkResult createStatus = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
    if (createStatus != VK_SUCCESS) {
        LOGE("Failed to create instance! Status = %d\n", static_cast<int>(createStatus));
        fflush(stdout);
        throw std::runtime_error("Failed to create instance!");
    }
}

// К методам расширениям может не быть прямого доступа, поэтому создаем коллбек вручную
VkResult VulkanDevice::createDebugReportCallbackEXT(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                                    const VkAllocationCallbacks* pAllocator,
                                                    VkDebugReportCallbackEXT* pCallback) {
    // Запрашиваем адрес функции
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vulkanInstance,
                                                                          "vkCreateDebugReportCallbackEXT");

    if (func != nullptr) {
        return func(vulkanInstance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Убираем коллбек
void VulkanDevice::destroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback,
                                                 const VkAllocationCallbacks* pAllocator) {
    // Запрашиваем адрес функции
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vulkanInstance,
                                                                           "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(vulkanInstance, callback, pAllocator);
    }
}

// Устанавливаем коллбек для отладки
void VulkanDevice::setupDebugCallback() {
#ifdef VALIDATION_LAYERS_ENABLED
    // Структура с описанием коллбека
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    memset(&createInfo, 0, sizeof(VkDebugReportCallbackCreateInfoEXT));
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags =  VK_DEBUG_REPORT_ERROR_BIT_EXT |
                        VK_DEBUG_REPORT_WARNING_BIT_EXT |
                        VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT; // Что выводим в коллбек
    createInfo.pfnCallback = debugCallback;

    VkResult createCallbackStatus = createDebugReportCallbackEXT(&createInfo, nullptr, &vulkanDebugCallback);
    if (createCallbackStatus != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback!");
    }
#endif
}

/*void VulkanDevice::createSwapchain(){
    std::array<const char*, 1> deviceExtensions;
    deviceExtensions[0] = "VK_KHR_swapchain";
}*/