#include "VulkanDevice.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <set>
#include <array>
#include <vector>
#include <android/log.h>
#include "SupportFunctions.h"


#define VALIDATION_LAYERS_ENABLED

#define  LOG_TAG    "VulkanTest"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


VulkanDevice::VulkanDevice():
   vulkanInstance(VK_NULL_HANDLE){
}

VulkanDevice::~VulkanDevice() {
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
bool checkAllLayersAvailable(const std::vector<VkLayerProperties>& allLayers, const std::vector<const char*>& testLayers){
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

// Получаем доступные слои валидации устройства
std::vector<const char *> VulkanDevice::getPossibleDebugValidationLayers(){
#ifdef VALIDATION_LAYERS_ENABLED
    // Список всех слоев
    std::vector<VkLayerProperties> allValidationLayers = getAllValidationLayers();
    for(const VkLayerProperties& layerInfo: allValidationLayers){
        LOGD("Validation layer available: %s\n", layerInfo.layerName);
        fflush(stdout);
    }

    std::vector<const char*> result;
    result.push_back("VK_LAYER_LUNARG_standard_validation");
    if (!checkAllLayersAvailable(allValidationLayers, result)) {
        result.clear();
        result.push_back("VK_LAYER_LUNARG_image");
        result.push_back("VK_LAYER_GOOGLE_threading");
        result.push_back("VK_LAYER_LUNARG_parameter_validation");
        result.push_back("VK_LAYER_LUNARG_object_tracker");
        result.push_back("VK_LAYER_LUNARG_core_validation");
        result.push_back("VK_LAYER_GOOGLE_unique_objects");
        result.push_back("VK_LAYER_LUNARG_swapchain");

        if (!checkAllLayersAvailable(allValidationLayers, result)) {
            LOGD("Failed to get validation layers!\n");
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
    // Расширения инстанса, которые нужно будет использовать
    std::array<const char*, 2> instanceExtensions;
    instanceExtensions[0] = "VK_KHR_surface";
    instanceExtensions[1] = "VK_KHR_android_surface";

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

    // Структура настроек создания инстанса
    VkInstanceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());  // Включаем расширения
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    createInfo.enabledLayerCount = (uint32_t)validationLayers.size();     // Включаем стандартные слои валидации
    createInfo.ppEnabledLayerNames = validationLayers.data();

    // Непосредственно создание инстанса Vulkan
    VkResult createStatus = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
    if (createStatus != VK_SUCCESS) {
        LOGD("Failed to create instance! Status = %d\n", static_cast<int>(createStatus));
        fflush(stdout);
        throw std::runtime_error("Failed to create instance!");
    }
}

void VulkanDevice::createSwapchain(){
    std::array<const char*, 1> deviceExtensions;
    deviceExtensions[0] = "VK_KHR_swapchain";
}