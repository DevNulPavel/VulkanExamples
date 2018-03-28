#include "VulkanInstance.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>


VulkanInstance::VulkanInstance():
    _instance(VK_NULL_HANDLE)
#ifdef VALIDATION_LAYERS_ENABLED
    , _debugCallback(VK_NULL_HANDLE)
#endif
{
    
    createVulkanInstance();
    setupDebugCallback();
}

VulkanInstance::~VulkanInstance(){
#ifdef VALIDATION_LAYERS_ENABLED
    destroyDebugReportCallbackEXT(_debugCallback, nullptr);
#endif
    vkDestroyInstance(_instance, nullptr);
}

VkInstance VulkanInstance::getInstance() const{
    return _instance;
}

std::vector<const char*> VulkanInstance::getValidationLayers(){
    return _validationLayers;
}

std::vector<const char*> VulkanInstance::getInstanceExtensions(){
    return _instanceExtensions;
}

// Получаем все доступные слои валидации устройства
std::vector<VkLayerProperties> VulkanInstance::getAllValidationLayers(){
    // Количество уровней валидации
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    // Получаем доступные слои валидации
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    return availableLayers;
}

// Проверяем, что все запрошенные слои нам доступны
bool VulkanInstance::checkAllLayersInVectorAvailable(const std::vector<VkLayerProperties>& allLayers, const std::vector<const char*>& testLayers){
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
            printf("Layer %s not available!\n", layerName);
            return false;
        }
    }
    return true;
}

// Получаем доступные слои валидации устройства
std::vector<const char *> VulkanInstance::getPossibleDebugValidationLayers(){
#ifdef VALIDATION_LAYERS_ENABLED
    // Список всех слоев
    std::vector<VkLayerProperties> allValidationLayers = getAllValidationLayers();
    for(const VkLayerProperties& layerInfo: allValidationLayers){
        printf("Validation layer available: %s (%s)\n", layerInfo.layerName, layerInfo.description);
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
            printf("Failed to get validation layers!\n");
            throw std::runtime_error("Failed to create instance!");
        }
    }
    
    return result;
#else
    return std::vector<const char*>();
#endif
}

// Список необходимых расширений инстанса приложения
std::map<std::string, std::vector<VkExtensionProperties>> VulkanInstance::getAllExtentionsNames(const std::vector<const char *>& layersNames){
    std::map<std::string, std::vector<VkExtensionProperties>> result;
    
    for (const char* layerName: layersNames) {
        std::vector<VkExtensionProperties> layerExtentions;
        
        // Количество расширений доступных
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);
        
        // Получаем расширения
        layerExtentions.resize(extensionCount);
        vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, layerExtentions.data());
        
        std::vector<VkExtensionProperties>& propsArray = result[layerName];
        propsArray.insert(propsArray.end(), layerExtentions.begin(), layerExtentions.end());
    }
    
    return result;
}

// Список всех расширений в леерах
void VulkanInstance::printAllExtentionsAtLayers(const std::vector<const char *>& layersNames){
    std::map<std::string, std::vector<VkExtensionProperties>> allExtentions = getAllExtentionsNames(layersNames);
    for(const std::pair<std::string, std::vector<VkExtensionProperties>>& extentionInfo: allExtentions){
        for (const VkExtensionProperties& property: extentionInfo.second) {
            printf("Extention at layer %s available: %s\n", extentionInfo.first.c_str(), property.extensionName);
        }
    }
}

// Список необходимых расширений инстанса приложения
std::vector<const char*> VulkanInstance::getRequiredInstanceExtentionNames(){
    // Количество требуемых GLFW расширений и список расширений
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector<const char*> result;
    // Формируем список расширений
    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        result.push_back(glfwExtensions[i]);
        printf("Extention - Required GLFW extention name: %s\n", glfwExtensions[i]);
        fflush(stdout);
    }
#ifdef __APPLE__
    // Optional
    result.push_back("VK_MVK_moltenvk");
    printf("Extention - Required MoltenVK extention name: %s\n", "VK_MVK_moltenvk");
    fflush(stdout);
#endif
    
#ifdef VALIDATION_LAYERS_ENABLED
    result.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    printf("Extention - Required debug extention name: %s\n", VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    fflush(stdout);
#endif
    return result;
}

// Создание инстанса Vulkan
void VulkanInstance::createVulkanInstance(){
    // Запрашиваем возможные слои валидации
    _validationLayers = getPossibleDebugValidationLayers();
    
    // Выводим расширения в слоях валидации
    printAllExtentionsAtLayers(_validationLayers);
    
    // Список требуемых расширений
    _instanceExtensions = getRequiredInstanceExtentionNames();
    
    // Структура с настройками приложения Vulkan
    VkApplicationInfo appInfo = {};
    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);  // Указываем используемую версию Vulkan, VK_API_VERSION_1_0
    
    // Структура настроек создания инстанса
    VkInstanceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());  // Включаем стандартные слои валидации
    createInfo.ppEnabledLayerNames = _validationLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(_instanceExtensions.size());  // Включаем расширения
    createInfo.ppEnabledExtensionNames = _instanceExtensions.data();
    
    // Непосредственно создание инстанса Vulkan
    VkResult createStatus = vkCreateInstance(&createInfo, nullptr, &_instance);
    if (createStatus != VK_SUCCESS) {
        printf("Failed to create instance! Status = %d\n", static_cast<int>(createStatus));
        fflush(stdout);
        throw std::runtime_error("Failed to create instance!");
    }
}

// К методам расширениям может не быть прямого доступа, поэтому создаем коллбек вручную
VkResult VulkanInstance::createDebugReportCallbackEXT(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                                      const VkAllocationCallbacks* pAllocator,
                                                      VkDebugReportCallbackEXT* pCallback) {
    // Запрашиваем адрес функции
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(_instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Убираем коллбек
void VulkanInstance::destroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback,
                                                   const VkAllocationCallbacks* pAllocator) {
    // Запрашиваем адрес функции
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(_instance, callback, pAllocator);
    }
}


// Отладочный коллбек
#ifdef VALIDATION_LAYERS_ENABLED
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunction(VkDebugReportFlagsEXT flags,
                                                            VkDebugReportObjectTypeEXT objType,
                                                            uint64_t obj,
                                                            size_t location,
                                                            int32_t code,
                                                            const char* layerPrefix,
                                                            const char* msg,
                                                            void* userData) {
    std::string messageType;
    if((flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0) {
        messageType = "INFO";
    }else if((flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0) {
        messageType = "WARNING";
    } else if((flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0){
        messageType = "PERFORMANCE";
    } else if((flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0) {
        messageType = "DEBUG";
    } else if((flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0) {
        messageType = "ERROR";
    }
    
    printf("Debug message (%s): %s\n", messageType.c_str(), msg);
    fflush(stdout);
    return VK_FALSE;
}
#endif

// Устанавливаем коллбек для отладки
void VulkanInstance::setupDebugCallback() {
#ifdef VALIDATION_LAYERS_ENABLED
    // Структура с описанием коллбека
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    memset(&createInfo, 0, sizeof(VkDebugReportCallbackCreateInfoEXT));
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
    VK_DEBUG_REPORT_WARNING_BIT_EXT |
    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
    VK_DEBUG_REPORT_INFORMATION_BIT_EXT; // Типы коллбеков
    createInfo.pfnCallback = debugCallbackFunction;
    
    if (createDebugReportCallbackEXT(&createInfo, nullptr, &_debugCallback) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback!");
    }
#endif
}
