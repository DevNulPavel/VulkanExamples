#include "VulkanDevice.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <set>
#include <array>
#include <vector>
#include <map>
#include <tuple>
#include "SupportFunctions.h"


#ifndef NDEBUG
    #define VALIDATION_LAYERS_ENABLED
#endif

#define DEVICE_REQUIRED_EXTENTIONS_COUNT 1
const char* DEVICE_REQUIRED_EXTENTIONS[DEVICE_REQUIRED_EXTENTIONS_COUNT] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


////////////////////////////////////////////////////////////////////////////////////////////////////////

// Отладочный коллбек
#ifdef VALIDATION_LAYERS_ENABLED
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
                                                    VkDebugReportObjectTypeEXT objType,
                                                    uint64_t obj,
                                                    size_t location,
                                                    int32_t code,
                                                    const char* layerPrefix,
                                                    const char* msg,
                                                    void* userData) {
    LOGE("Validation layer message %s: %s\n", layerPrefix, msg);
    return VK_FALSE;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

VulkanDevice::VulkanDevice(ANativeWindow* androidNativeWindow, uint32_t windowW, uint32_t windowH):
    windowWidth(windowW),
    windowHeight(windowH),
    vulkanInstance(VK_NULL_HANDLE),
    vulkanDebugCallback(VK_NULL_HANDLE),
    vulkanPhysicalDevice(VK_NULL_HANDLE),
    vulkanLogicalDevice(VK_NULL_HANDLE),
    vulkanGraphicsQueue(VK_NULL_HANDLE),
    vulkanPresentQueue(VK_NULL_HANDLE){

    createVulkanInstance();
    setupDebugCallback();
    createSurface(androidNativeWindow);
    selectPhysicalDevice();
    createLogicalDeviceAndQueue();
}

VulkanDevice::~VulkanDevice() {
    vkDestroyDevice(vulkanLogicalDevice, nullptr);
    vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, nullptr);
    #ifdef VALIDATION_LAYERS_ENABLED
        destroyDebugReportCallbackEXT(vulkanDebugCallback, nullptr);
    #endif
    vkDestroyInstance(vulkanInstance, nullptr);

    LOGE("Vulkan device destroyed");
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
bool VulkanDevice::checkAllLayersInVectorAvailable(const std::vector<VkLayerProperties>& allLayers, const std::vector<const char*>& testLayers){
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
            LOGE("Layer %s not available!\n", layerName);
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
        LOGE("Validation layer available: %s (%s)\n", layerInfo.layerName, layerInfo.description);
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
            //throw std::runtime_error("Failed to create instance!");
            result.clear();
        }
    }

    return result;
#else
    return std::vector<const char*>();
#endif
}

// Список необходимых расширений инстанса приложения
std::map<std::string, std::vector<VkExtensionProperties>> VulkanDevice::getAllExtentionsNames(const std::vector<const char *>& layersNames){
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
void VulkanDevice::printAllExtentionsAtLayers(const std::vector<const char *>& layersNames){
    std::map<std::string, std::vector<VkExtensionProperties>> allExtentions = getAllExtentionsNames(layersNames);
    for(const std::pair<std::string, std::vector<VkExtensionProperties>>& extentionInfo: allExtentions){
        for (const VkExtensionProperties& property: extentionInfo.second) {
            LOGE("Extention at layer %s available: %s\n", extentionInfo.first.c_str(), property.extensionName);
        }
    }
}

// Список необходимых расширений инстанса приложения
std::vector<const char*> VulkanDevice::getRequiredInstanceExtentionNames(){
    std::vector<const char*> result;
    result.push_back("VK_KHR_surface");
    result.push_back("VK_KHR_android_surface");
    #ifdef VALIDATION_LAYERS_ENABLED
        result.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    #endif
    return result;
}

void VulkanDevice::createVulkanInstance(){

    // Запрашиваем возможные слои валидации
    vulkanValidationLayers = getPossibleDebugValidationLayers();

    // Выводим расширения в слоях валидации
    printAllExtentionsAtLayers(vulkanValidationLayers);

    // Список требуемых расширений
    vulkanInstanceExtensions = getRequiredInstanceExtentionNames();

    // Структура с настройками приложения Vulkan
    VkApplicationInfo appInfo = {};
    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;  // Указываем используемую версию Vulkan

    // Структура настроек создания инстанса
    VkInstanceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(vulkanValidationLayers.size());     // Включаем стандартные слои валидации
    createInfo.ppEnabledLayerNames = vulkanValidationLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(vulkanInstanceExtensions.size());  // Включаем расширения
    createInfo.ppEnabledExtensionNames = vulkanInstanceExtensions.data();

    // Непосредственно создание инстанса Vulkan
    VkResult createStatus = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
    if (createStatus != VK_SUCCESS) {
        LOGE("Failed to create instance! Status = %d\n", static_cast<int>(createStatus));
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
                            //VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT; // Что выводим в коллбек
        createInfo.pfnCallback = debugCallback;

        VkResult createCallbackStatus = createDebugReportCallbackEXT(&createInfo, nullptr, &vulkanDebugCallback);
        if (createCallbackStatus != VK_SUCCESS) {
            LOGE("Failed to set up debug callback!");
            throw std::runtime_error("Failed to set up debug callback!");
        }
    #endif
}

// Создаем плоскость отрисовки
void VulkanDevice::createSurface(ANativeWindow* androidNativeWindow){
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
    memset(&surfaceCreateInfo, 0, sizeof(VkAndroidSurfaceCreateInfoKHR));
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = nullptr;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.window = androidNativeWindow; // Нативный Surface на андроиде

    VkResult status = vkCreateAndroidSurfaceKHR(vulkanInstance, &surfaceCreateInfo, nullptr, &vulkanSurface);
    if (status != VK_SUCCESS) {
        LOGE("Failed to create surface!");
        throw std::runtime_error("Failed to create surface!");
    }
}

// Все возможные расширения физического устройства
std::vector<VkExtensionProperties> VulkanDevice::getAllPhysicalDeviceExtentions(VkPhysicalDevice device){
    // Получаем количество расширений
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // Получаем расширения
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    return availableExtensions;
}

// Проверяем, поддерживает ли девайс цепочку свопинга
bool VulkanDevice::checkPhysicalDeviceRequiredExtensionSupport(VkPhysicalDevice device) {
    std::vector<VkExtensionProperties> availableExtensions = getAllPhysicalDeviceExtentions(device);

    // Список требуемых расширений - смена кадров
    std::set<std::string> requiredExtensions(DEVICE_REQUIRED_EXTENTIONS,
                                             DEVICE_REQUIRED_EXTENTIONS + DEVICE_REQUIRED_EXTENTIONS_COUNT);

    // Пытаемся убрать из списка требуемых расширений возможные
    for (const auto& extension : availableExtensions) {
        LOGE("Available physical device extention: %s\n", extension.extensionName);
        requiredExtensions.erase(extension.extensionName);
    }

    // Если пусто, значит поддерживается
    return requiredExtensions.empty();
}

// Запрашиваем поддержку свопачейна изображений на экране
SwapChainSupportDetails VulkanDevice::queryPhysicalDeviceSwapChainSupport(VkPhysicalDevice device) {

    SwapChainSupportDetails details;

    // Получаем возможности
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkanSurface, &details.capabilities);

    // Запрашиваем поддерживаемые форматы буффера цвета
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanSurface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanSurface, &formatCount, details.formats.data());
    }

    // Запрашиваем поддерживаемые типы отображения кадра
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanSurface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// Список семейств очередей на конкретном физическом устройстве
std::vector<VkQueueFamilyProperties> VulkanDevice::getPhysicalDeviceAllQueueFamilies(VkPhysicalDevice device){
    // Запрашиваем количество возможных семейств очередей в устройстве
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Запрашиваем список семейств очередей устройства
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    return queueFamilies;
}

// Для данного устройства ищем семейства очередей отрисовки
FamiliesQueueIndexes VulkanDevice::findPhysicalDeviceQueueFamiliesIndex(VkPhysicalDevice device) {
    std::vector<VkQueueFamilyProperties> queueFamilies = getPhysicalDeviceAllQueueFamilies(device);

    FamiliesQueueIndexes result;

    // Подбираем информацию об очередях
    uint32_t i = 0;
    for (const VkQueueFamilyProperties& queueFamily: queueFamilies) {

        // Для группы очередей отрисовки проверяем, что там есть очереди + есть очередь отрисовки
        if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            result.renderQueueFamilyIndex = i;
            result.renderQueueFamilyQueuesCount = queueFamily.queueCount;
        }

        // Провеяем, может является ли данная очередь - очередью отображения
        VkBool32 presentSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkanSurface, &presentSupport);
        if ((queueFamily.queueCount > 0) && presentSupport) {
            result.presentQueueFamilyIndex = i;
            result.presentQueueFamilyQueuesCount = queueFamily.queueCount;
        }

        // Нашли очереди
        if (result.isComplete()) {
            return result;
        }

        i++;
    }

    return result;
}

// Оценка производительности и пригодности конкретной GPU
int VulkanDevice::ratePhysicalDeviceScore(VkPhysicalDevice device) {
    // Свойства физического устройства
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Фичи физического устройства
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    LOGE("Test GPU with name: %s, API version: %d\n", deviceProperties.deviceName, deviceProperties.apiVersion);

    int score = 0;

    // Дискретная карта обычно имеет большую производительность
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Максимальный размер текстур
    score += deviceProperties.limits.maxImageDimension2D;

    // Нету геометрического шейдера??
    /*if (!deviceFeatures.geometryShader) {
        return 0;
    }*/

    return score;
}

// Инициализация физического устройства
void VulkanDevice::selectPhysicalDevice(){
    // Получаем количество GPU
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

    // Есть ли вообще карты?
    if (deviceCount == 0) {
        LOGE("Failed to find GPUs with Vulkan support!");
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    // Получаем список устройств
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());

    // Используем Map для автоматической сортировки по производительности
    std::map<int, std::tuple<VkPhysicalDevice, FamiliesQueueIndexes, SwapChainSupportDetails>> candidates;

    // Перебираем GPU для поиска подходящего устройства
    for (const VkPhysicalDevice& device: devices) {
        // Смотрим - есть ли у данного устройства поддержка свопа кадров в виде расширения?
        bool swapchainExtentionSupported = checkPhysicalDeviceRequiredExtensionSupport(device);
        if(!swapchainExtentionSupported){
            continue;
        }

        // Проверяем, поддержку свопчейна у девайса, есть ли форматы и режимы отображения
        SwapChainSupportDetails swapChainSupport = queryPhysicalDeviceSwapChainSupport(device);
        bool swapChainValid = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if (!swapChainValid) {
            continue;
        }

        // Получаем индекс группы очередй отрисовки
        FamiliesQueueIndexes familiesInfo = findPhysicalDeviceQueueFamiliesIndex(device);
        if (familiesInfo.isComplete()) {
            // Оцениваем возможности устройства
            int score = ratePhysicalDeviceScore(device);
            candidates[score] = std::tuple<VkPhysicalDevice, FamiliesQueueIndexes, SwapChainSupportDetails>(device, familiesInfo, swapChainSupport);
        }
    }

    // Есть ли вообще карты?
    if (candidates.size() == 0) {
        throw std::runtime_error("No picked GPU physical devices!");
    }

    // Получаем наилучший вариант GPU
    if (candidates.begin()->first > 0) {
        vulkanPhysicalDevice = std::get<0>(candidates.begin()->second);
        vulkanFamiliesQueueIndexes = std::get<1>(candidates.begin()->second);
        vulkanSwapChainSupportDetails = std::get<2>(candidates.begin()->second);
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

// Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
void VulkanDevice::createLogicalDeviceAndQueue() {
    // Только уникальные индексы семейств очередей
    std::set<int32_t> uniqueQueueFamilies;
    uniqueQueueFamilies.insert(vulkanFamiliesQueueIndexes.presentQueueFamilyIndex);
    uniqueQueueFamilies.insert(vulkanFamiliesQueueIndexes.renderQueueFamilyIndex);

    // Определяем количество создаваемых очередей
    uint32_t createQueuesCount = 1;
    if (vulkanFamiliesQueueIndexes.renderQueueFamilyIndex == vulkanFamiliesQueueIndexes.presentQueueFamilyIndex) {
        // Если в одном семействе больше одной очереди - берем разные, иначе одну
        if (vulkanFamiliesQueueIndexes.renderQueueFamilyQueuesCount > 1) {
            createQueuesCount = 2;
        }else{
            createQueuesCount = 1;
        }
    }else{
        createQueuesCount = 1;
    }

    // Создаем экземпляры настроек создания очереди
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const float queuePriority[2] = {1.0f, 0.0f};
    for (int32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamily);  // Индекс семейства очередей
        queueCreateInfo.queueCount = createQueuesCount;                     // Количество очередей
        queueCreateInfo.pQueuePriorities = queuePriority;  // Приоритет очереди

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Нужные фичи устройства (ничего не указываем)
    VkPhysicalDeviceFeatures deviceFeatures = {};
    memset(&deviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

    // Конфиг создания девайса
    VkDeviceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data(); // Информация о создаваемых на девайсе очередях
    createInfo.pEnabledFeatures = &deviceFeatures;          // Информация о фичах устройства
    createInfo.enabledExtensionCount = DEVICE_REQUIRED_EXTENTIONS_COUNT;
    createInfo.ppEnabledExtensionNames = DEVICE_REQUIRED_EXTENTIONS;     // Список требуемых расширений устройства
    createInfo.enabledLayerCount = static_cast<uint32_t>(vulkanValidationLayers.size());
    createInfo.ppEnabledLayerNames = vulkanValidationLayers.data();

    // Пробуем создать логический девайс на конкретном физическом
    VkResult createStatus = vkCreateDevice(vulkanPhysicalDevice, &createInfo, nullptr, &vulkanLogicalDevice);
    if (createStatus != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    // Если в одном семействе больше одной очереди - берем разные, иначе одну
    if (createQueuesCount >= 2) {
        vkGetDeviceQueue(vulkanLogicalDevice, static_cast<uint32_t>(vulkanFamiliesQueueIndexes.renderQueueFamilyIndex), 0, &vulkanGraphicsQueue);
        vkGetDeviceQueue(vulkanLogicalDevice, static_cast<uint32_t>(vulkanFamiliesQueueIndexes.presentQueueFamilyIndex), 1, &vulkanPresentQueue);
    }else{
        vkGetDeviceQueue(vulkanLogicalDevice, static_cast<uint32_t>(vulkanFamiliesQueueIndexes.renderQueueFamilyIndex), 0, &vulkanGraphicsQueue);
        vkGetDeviceQueue(vulkanLogicalDevice, static_cast<uint32_t>(vulkanFamiliesQueueIndexes.presentQueueFamilyIndex), 0, &vulkanPresentQueue);
    }
}
