// Windows
#ifdef _MSVC_LANG 
    #define NOMINMAX
    #include <windows.h>
#endif

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <limits>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// STB image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// TinyObj
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "CommonDefines.h"
#include "CommonConstants.h"
#include "Vertex.h"
#include "Figures.h"
#include "UniformBuffer.h"


#define VALIDATION_LAYERS_COUNT 1
#ifdef __APPLE__
const char* VALIDATION_LAYERS[VALIDATION_LAYERS_COUNT] = { "MoltenVK" };
#else
const char* VALIDATION_LAYERS[VALIDATION_LAYERS_COUNT] = { "VK_LAYER_LUNARG_standard_validation" };
#endif

#define DEVICE_REQUIRED_EXTENTIONS_COUNT 1
const char* DEVICE_REQUIRED_EXTENTIONS[DEVICE_REQUIRED_EXTENTIONS_COUNT] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#define APPLICATION_SAMPLING_VALUE VK_SAMPLE_COUNT_1_BIT


GLFWwindow* window = nullptr;
VkInstance vulkanInstance = VK_NULL_HANDLE;
VkSurfaceKHR vulkanSurface = VK_NULL_HANDLE;
VkDebugReportCallbackEXT vulkanDebugCallback = VK_NULL_HANDLE;
VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
int vulkanRenderQueueFamilyIndex = -1;
int vulkanPresentQueueFamilyIndex = -1;
VkDevice vulkanLogicalDevice = VK_NULL_HANDLE;
VkQueue vulkanGraphicsQueue = VK_NULL_HANDLE;
VkQueue vulkanPresentQueue = VK_NULL_HANDLE;
VkSwapchainKHR vulkanSwapchain = VK_NULL_HANDLE;
std::vector<VkImage> vulkanSwapChainImages;
VkFormat vulkanSwapChainImageFormat = VK_FORMAT_UNDEFINED;
VkFormat vulkanDepthFormat = VK_FORMAT_UNDEFINED;
VkExtent2D vulkanSwapChainExtent = {0, 0};
std::vector<VkImageView> vulkanSwapChainImageViews;
VkShaderModule vulkanVertexShader = VK_NULL_HANDLE;
VkShaderModule vulkanFragmentShader = VK_NULL_HANDLE;
VkRenderPass vulkanRenderPass = VK_NULL_HANDLE;
VkDescriptorSetLayout vulkanDescriptorSetLayout = VK_NULL_HANDLE;
VkPipelineLayout vulkanPipelineLayout = VK_NULL_HANDLE;
VkPipeline vulkanPipeline = VK_NULL_HANDLE;
std::vector<VkFramebuffer> vulkanSwapChainFramebuffers;
VkCommandPool vulkanCommandPool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> vulkanCommandBuffers;
VkSemaphore vulkanImageAvailableSemaphore = VK_NULL_HANDLE;
VkSemaphore vulkanRenderFinishedSemaphore = VK_NULL_HANDLE;
VkBuffer vulkanVertexBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanVertexBufferMemory = VK_NULL_HANDLE;
VkBuffer vulkanIndexBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanIndexBufferMemory = VK_NULL_HANDLE;
VkBuffer vulkanUniformStagingBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanUniformStagingBufferMemory = VK_NULL_HANDLE;
VkBuffer vulkanUniformBuffer = VK_NULL_HANDLE;
VkDeviceMemory vulkanUniformBufferMemory = VK_NULL_HANDLE;
VkDescriptorPool vulkanDescriptorPool = VK_NULL_HANDLE;
VkDescriptorSet vulkanDescriptorSet = VK_NULL_HANDLE;
VkImage vulkanTextureImage = VK_NULL_HANDLE;
VkDeviceMemory vulkanTextureImageMemory = VK_NULL_HANDLE;
VkImageView vulkanTextureImageView = VK_NULL_HANDLE;
VkSampler vulkanTextureSampler = VK_NULL_HANDLE;
VkImage vulkanDepthImage = VK_NULL_HANDLE;
VkDeviceMemory vulkanDepthImageMemory = VK_NULL_HANDLE;
VkImageView vulkanDepthImageView = VK_NULL_HANDLE;
std::vector<Vertex> vulkanVertices;
std::vector<uint32_t> vulkanIndices;
size_t vulkanTotalVertexesCount = 0;
size_t vulkanTotalIndexesCount = 0;


////////////////////////////////////////////////////////////////////////////////////////////////

float rotateAngle = 0.0f;

////////////////////////////////////////////////////////////////////////////////////////////////

struct FamiliesQueueIndexes {
    int renderQueueFamilyIndex;     // Индекс семейства очередей отрисовки
    int presentQueueFamilyIndex;    // Индекс семейства очередей отображения
    
    FamiliesQueueIndexes(){
        renderQueueFamilyIndex = -1;
        presentQueueFamilyIndex = -1;
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

////////////////////////////////////////////////////////////////////////////////////////////////

// Читаем побайтово файлик
std::vector<char> readFile(const std::string& filename) {
    // Открываем файлик в бинарном режиме чтения + чтение с конца
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    
    // Получаем размер файлика
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    
    // Переходим в начало файла и читаем данные
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    
    file.close();
    
    return buffer;
}

// К методам расширениям может не быть прямого доступа, поэтому создаем коллбек вручную
VkResult createDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    // Запрашиваем адрес функции
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Убираем коллбек
void destroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    // Запрашиваем адрес функции
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

// Получаем расширения Vulkan
std::vector<const char*> getRequiredExtensions() {
    std::vector<const char*> extensions;
    
    // Количество требуемых GLFW расширений и список расширений
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    // Формируем список расширений
    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
        printf("Required GLFW extention name: %s\n", glfwExtensions[i]);
        fflush(stdout);
    }
    #ifdef VALIDATION_LAYERS_ENABLED
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    #endif
    
    return extensions;
}

// Проверка наличия уровней валидации
bool checkValidationLayerSupport() {
    #ifdef VALIDATION_LAYERS_ENABLED
        // Количество уровней валидации
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
        // Получаем доступные слои валидации
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
        // Проверяем, что запрошенные слои валидации доступны
        for(int i = 0; i < VALIDATION_LAYERS_COUNT; i++) {
            const char* layerName = VALIDATION_LAYERS[i];
            bool layerFound = false;
            
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            
            if (!layerFound) {
                return false;
            }
        }
    #endif
    return true;
}

// Получаем все доступные слои валидации устройства
std::vector<VkLayerProperties> getAllValidationLayers(){
    // Количество уровней валидации
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    // Получаем доступные слои валидации
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    return availableLayers;
}

// Создание инстанса Vulkan
void createVulkanInstance(){
    #ifdef VALIDATION_LAYERS_ENABLED
        // Проверяем доступность слоев валидации
        if (!checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
    #endif
    
    // Структура с настройками приложения Vulkan
    VkApplicationInfo appInfo = {};
    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;  // Указываем используемую версию Vulkan
    
    // Получаем список требуемых GLFW расширений
    std::vector<const char*> extensions = getRequiredExtensions();
    
    // Структура настроек создания инстанса
    VkInstanceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());  // Включаем расширения
    createInfo.ppEnabledExtensionNames = extensions.data();
    #ifdef VALIDATION_LAYERS_ENABLED
        // Включаем стандартные слои валидации
        createInfo.enabledLayerCount = VALIDATION_LAYERS_COUNT;
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
    #else
        // Не испольузем валидации
        createInfo.enabledLayerCount = 0;
    #endif
    
    // Непосредственно создание инстанса Vulkan
    VkResult createStatus = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
    if (createStatus != VK_SUCCESS) {
        printf("Failed to create instance! Status = %d\n", static_cast<int>(createStatus));
		fflush(stdout);
        throw std::runtime_error("Failed to create instance!");
    }
}

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
    printf("Validation layer: %s\n", msg);
    fflush(stdout);
    return VK_FALSE;
}
#endif

// Устанавливаем коллбек для отладки
void setupDebugCallback() {
    #ifdef VALIDATION_LAYERS_ENABLED
        // Структура с описанием коллбека
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        memset(&createInfo, 0, sizeof(VkDebugReportCallbackCreateInfoEXT));
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT; // Типы коллбеков
        createInfo.pfnCallback = debugCallback;
    
        if (createDebugReportCallbackEXT(vulkanInstance, &createInfo, nullptr, &vulkanDebugCallback) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug callback!");
        }
    #endif
}

// Создаем плоскость отрисовки GLFW
void createDrawSurface() {
    if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &vulkanSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

// Для данного устройства ищем очереди отрисовки
FamiliesQueueIndexes findQueueFamiliesIndexInDevice(VkPhysicalDevice device) {
    // Запрашиваем количество возможных семейств очередей в устройстве
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    // Запрашиваем список семейств очередей устройства
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    FamiliesQueueIndexes result;
    
    // Подбираем информацию об очередях
    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily: queueFamilies) {
        
        // Для группы очередей отрисовки проверяем, что там есть очереди + есть очередь отрисовки
        if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            result.renderQueueFamilyIndex = i;
        }
        
        // Провеяем, может является ли данная очередь - очередью отображения
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkanSurface, &presentSupport);
        if ((queueFamily.queueCount > 0) && presentSupport) {
            result.presentQueueFamilyIndex = i;
        }
        
        // Нашли очереди
        if (result.isComplete()) {
            return result;
        }
        
        i++;
    }
    
    return result;
}

// Проверяем, поддерживает ли девайс цепочку свопинга
bool checkDeviceRequiredExtensionSupport(VkPhysicalDevice device) {
    // Получаем количество расширений
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    // Получаем расширения
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    // Список требуемых расширений - смена кадров
    std::set<std::string> requiredExtensions(DEVICE_REQUIRED_EXTENTIONS, DEVICE_REQUIRED_EXTENTIONS + DEVICE_REQUIRED_EXTENTIONS_COUNT);
    
    // Пытаемся убрать из списка требуемых расширений возможные
    for (const auto& extension : availableExtensions) {
        printf("Available extention: %s\n", extension.extensionName);
        fflush(stdout);
        requiredExtensions.erase(extension.extensionName);
    }
    
    // Если пусто, значит поддерживается
    return requiredExtensions.empty();
}

// Оценка производительности и пригодности конкретной GPU
int rateDeviceScore(VkPhysicalDevice device) {
    // Свойства физического устройства
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    
    // Фичи физического устройства
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
    printf("Test GPU with name: %s, API version: %d\n", deviceProperties.deviceName, deviceProperties.apiVersion);
    fflush(stdout);

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

// Запрашиваем поддержку свопачейна изображений на экране
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    // Получаем возможности
    SwapChainSupportDetails details;
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

// Выбираем нужный формат кадра
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 0) {
        throw std::runtime_error("No available color formats!");
    }
    
    // Выбираем конкретный стандартный формат, если Vulkan не хочет ничего конкретного
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    
    // Иначе - перебираем список в поисках лучшего
    for (const auto& availableFormat : availableFormats) {
        if ((availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return availableFormat;
        }
    }
    
    // Если не нашли нужный - просто выбираем первый формат
    return availableFormats[0];
}

// Выбор режима представления кадров из буффера
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    // Проверяем, можно ли использовать тройную буфферизацию??
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    
    // Если нет - просто двойная буфферизация
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Выбираем размер кадра-свопа
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    
    VkExtent2D actualExtent = {WINDOW_WIDTH, WINDOW_HEIGHT};
    
    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
    
    return actualExtent;
}

// Дергаем видеокарту
void pickPhysicalDevice() {
    // Получаем количество GPU
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);
    
    // Есть ли вообще карты?
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    
    // Получаем список устройств
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());
    
    // Используем Map для автоматической сортировки по производительности
    std::map<int, std::pair<VkPhysicalDevice, FamiliesQueueIndexes>> candidates;
    
    // Перебираем GPU для поиска подходящего устройства
    for (const VkPhysicalDevice& device: devices) {
        // Смотрим - есть ли у данного устройства поддержка свопа кадров в виде расширения?
        bool swapchainExtentionSupported = checkDeviceRequiredExtensionSupport(device);
        if(swapchainExtentionSupported == false){
            continue;
        }
        
        // Проверяем, поддержку свопчейна у девайса
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        bool swapChainValid = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if (swapChainValid == false) {
            continue;
        }
        
        // Получаем индекс группы очередй отрисовки
        FamiliesQueueIndexes familiesFound = findQueueFamiliesIndexInDevice(device);
        if (familiesFound.isComplete()) {
            // Оцениваем возможности устройства
            int score = rateDeviceScore(device);
            candidates[score] = std::pair<VkPhysicalDevice,FamiliesQueueIndexes>(device, familiesFound);
        }
    }
    
    // Есть ли вообще карты?
    if (candidates.size() == 0) {
        throw std::runtime_error("No picked GPU physical devices!");
    }
    
    // Получаем наилучший вариант GPU
    if (candidates.begin()->first > 0) {
        vulkanPhysicalDevice = candidates.begin()->second.first;
        vulkanRenderQueueFamilyIndex = candidates.begin()->second.second.renderQueueFamilyIndex;
        vulkanPresentQueueFamilyIndex = candidates.begin()->second.second.presentQueueFamilyIndex;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

// Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
void createLogicalDeviceAndQueue() {
    // Только уникальные индексы очередей
    std::set<int> uniqueQueueFamilies = {vulkanRenderQueueFamilyIndex, vulkanPresentQueueFamilyIndex};
    
    // Создаем экземпляры настроек создания очереди
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Нужные фичи устройства (ничего не указываем)
    VkPhysicalDeviceFeatures deviceFeatures = {};
    memset(&deviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
    
    // Конфиг создания девайса
    VkDeviceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data(); // Информация о создаваемых на девайсе очередях
    createInfo.pEnabledFeatures = &deviceFeatures;          // Информация о фичах устройства
    createInfo.enabledExtensionCount = DEVICE_REQUIRED_EXTENTIONS_COUNT;
    createInfo.ppEnabledExtensionNames = DEVICE_REQUIRED_EXTENTIONS;     // Список требуемых расширений устройства
    #ifdef VALIDATION_LAYERS_ENABLED
        createInfo.enabledLayerCount = VALIDATION_LAYERS_COUNT;
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
    #else
        createInfo.enabledLayerCount = 0;
    #endif
    
    // Пробуем создать логический девайс на конкретном физическом
    VkResult createStatus = vkCreateDevice(vulkanPhysicalDevice, &createInfo, nullptr, &vulkanLogicalDevice);
    if (createStatus != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }
    
    // Создаем очередь из девайса
    vkGetDeviceQueue(vulkanLogicalDevice, vulkanRenderQueueFamilyIndex, 0, &vulkanGraphicsQueue);
    vkGetDeviceQueue(vulkanLogicalDevice, vulkanPresentQueueFamilyIndex, 0, &vulkanPresentQueue);
}

// Создание логики смены кадров
void createSwapChain() {
    // Запрашиваем информацию о свопчейне
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulkanPhysicalDevice);
    
    // Выбираем подходящие форматы пикселя, режима смены кадров, размеры кадра
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    
    // Получаем количество изображений в смене кадров, +1 для возможности создания тройной буфферизации
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    // Значение 0 для maxImageCount означает, что объем памяти не ограничен
    if ((swapChainSupport.capabilities.maxImageCount > 0) && (imageCount > swapChainSupport.capabilities.maxImageCount)) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    // Структура настроек создания Swapchain
    VkSwapchainCreateInfoKHR createInfo = {};
    memset(&createInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vulkanSurface;         // Плоскость отрисовки текущего окна
    createInfo.minImageCount = imageCount;      // Количество изображений в буффере
    createInfo.imageFormat = surfaceFormat.format; // Формат отображения
    createInfo.imageColorSpace = surfaceFormat.colorSpace;  // Цветовое пространство
    createInfo.imageExtent = extent;    // Границы окна
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // Картинки используются в качестве буффера цвета
    
    // Если у нас разные очереди для рендеринга и отображения -
    if (vulkanRenderQueueFamilyIndex != vulkanPresentQueueFamilyIndex) {
        uint32_t queueFamilyIndices[] = {(uint32_t)vulkanRenderQueueFamilyIndex, (uint32_t)vulkanPresentQueueFamilyIndex};
        // Изображение принадлежит одному семейству в один момент времени и должно быть явно передано другому семейству. Данный вариант обеспечивает наилучшую производительность.
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        // Изображение может быть использовано несколькими семействами без явной передачи.
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;   // Предварительный трансформ перед отображением графики, VK_SURFACE_TRANSFORM_*
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // Должно ли изображение смешиваться с альфа каналом оконной системы? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    // Пересоздание свопчейна
    VkSwapchainKHR oldSwapChain = vulkanSwapchain;
    createInfo.oldSwapchain = oldSwapChain;
    
    // Создаем свопчейн
    VkResult createStatus = vkCreateSwapchainKHR(vulkanLogicalDevice, &createInfo, nullptr, &vulkanSwapchain);
    if (createStatus != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    // Удаляем старый свопчейн, если был, обазательно удаляется после создания нового
    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkanLogicalDevice, oldSwapChain, nullptr);
        oldSwapChain = VK_NULL_HANDLE;
    }
    
    // Получаем изображения для отображения
    uint32_t imagesCount = 0;
    vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapchain, &imagesCount, nullptr);
    
    vulkanSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapchain, &imagesCount, vulkanSwapChainImages.data());
    
    vulkanSwapChainImageFormat = surfaceFormat.format;
    vulkanSwapChainExtent = extent;
}

// Создание вью для изображения
void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) {
    // Удаляем старый объект, если есть
    if(imageView != VK_NULL_HANDLE){
        vkDestroyImageView(vulkanLogicalDevice, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }
    
    // Описание вьюшки
    VkImageViewCreateInfo viewInfo = {};
    memset(&viewInfo, 0, sizeof(VkImageViewCreateInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image; // Изображение
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 2D
    viewInfo.format = format;   // Формат вьюшки
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.subresourceRange.aspectMask = aspectFlags; // Использование вью текстуры
    viewInfo.subresourceRange.baseMipLevel = 0; // 0 мипмаплевел
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    // Создаем имедж вью
    if (vkCreateImageView(vulkanLogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

// Создание вьюшек изображений буффера кадра свопчейна
void createImageViews() {
    // Удаляем старые, если есть
    if(vulkanSwapChainImageViews.size() > 0){
        for(const auto& imageView: vulkanSwapChainImageViews){
            vkDestroyImageView(vulkanLogicalDevice, imageView, nullptr);
        }
        vulkanSwapChainImageViews.clear();
    }
    
    // Ресайз массива
    vulkanSwapChainImageViews.resize(vulkanSwapChainImages.size());
    
    for (uint32_t i = 0; i < vulkanSwapChainImages.size(); i++) {
        // Создаем вьюшку с типом использования цвета
        createImageView(vulkanSwapChainImages[i], vulkanSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, vulkanSwapChainImageViews[i]);
    }
}

// Создаем дескриптор для буффера юниформов
void createDescriptorSetLayout() {
    // Лаяут для юниформ буффера
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    memset(&uboLayoutBinding, 0, sizeof(VkDescriptorSetLayoutBinding));
    uboLayoutBinding.binding = 0;   // Юниформ буффер находится на 0 позиции
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Тип - юниформ буффер
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Предназначен буффер толкьо для вершинного шейдера, можно подрубить другие
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
    
    // Лаяут для семплера
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    memset(&samplerLayoutBinding, 0, sizeof(VkDescriptorSetLayoutBinding));
    samplerLayoutBinding.binding = 1;   // Находится на 1й позиции
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;    // Семплер для текстуры
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Предназначено для фрагментного шейдера
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    
    // Биндинги
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {{uboLayoutBinding, samplerLayoutBinding}};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    
    // Создаем лаяут для шейдера
    if (vkCreateDescriptorSetLayout(vulkanLogicalDevice, &layoutInfo, nullptr, &vulkanDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

// Из байткода исходника создаем шейдерный модуль
void createShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule) {
    VkShaderModuleCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkShaderModuleCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = (uint32_t*)code.data();
    
    if (vkCreateShaderModule(vulkanLogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
}

// Создание рендер-прохода
void createRenderPass() {
    // Уничтожаем старый рендер пасс, если был уже
    if(vulkanRenderPass != VK_NULL_HANDLE){
        vkDestroyRenderPass(vulkanLogicalDevice, vulkanRenderPass, nullptr);
    }
    
    // Описание подсоединенного буффера цвета
    VkAttachmentDescription colorAttachment = {};
    memset(&colorAttachment, 0, sizeof(VkAttachmentDescription));
    colorAttachment.format = vulkanSwapChainImageFormat;    // Формат буфферов цвета кадра
    colorAttachment.samples = APPLICATION_SAMPLING_VALUE;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Что делать при начале работы с цветом?
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // После завершения что делать?
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Что делать с трафаретом при начале
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Что делать с трафаретом после
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // TODO: ???
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Изображение показывается в swap chain
    
    // Описание присоединенного буффера глубины
    VkAttachmentDescription depthAttachment = {};
    memset(&depthAttachment, 0, sizeof(VkAttachmentDescription));
    depthAttachment.format = vulkanDepthFormat; //  Формат
    depthAttachment.samples = APPLICATION_SAMPLING_VALUE; // Уровень семплирования
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Что делать при загрузке буффера глубины?
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Что делать после отрисовки с буффером глубины - буффер не рисуется, так что пофик на него
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    // Референс присоединенного цвета
    VkAttachmentReference colorAttachmentRef = {};
    memset(&colorAttachmentRef, 0, sizeof(VkAttachmentReference));
    colorAttachmentRef.attachment = 0;  // Аттачмент находится по 0му индексу в массиве attachments ниже
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   // Используется для цвета
    
    // Референс присоединенного буффера глубины
    VkAttachmentReference depthAttachmentRef = {};
    memset(&depthAttachmentRef, 0, sizeof(VkAttachmentReference));
    depthAttachmentRef.attachment = 1; // Аттачмент глубины находится по 1му индексу в массиве attachments ниже
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;   // Используется для глубины
    
    // Подпроход, всего один (наверное можно задействовать для постэффектов)
    VkSubpassDescription subPass = {};
    memset(&subPass, 0, sizeof(VkSubpassDescription));
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // Пайплайн будет использоваться для отрисовки графики
    subPass.colorAttachmentCount = 1;    // Один аттачмент цвета
    subPass.pColorAttachments = &colorAttachmentRef;    // Ref аттачмента цвета
    subPass.pDepthStencilAttachment = &depthAttachmentRef;  // Ref аттачмента глубины
    
    // Описание создания рендер-прохода
    std::array<VkAttachmentDescription, 2> attachments = {{colorAttachment, depthAttachment}};  // Индексы и использование указаны выше
    VkRenderPassCreateInfo renderPassInfo = {};
    memset(&renderPassInfo, 0, sizeof(VkRenderPassCreateInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;
    renderPassInfo.dependencyCount = 0;     // TODO: Зависимости?
    renderPassInfo.pDependencies = nullptr;
    
    // Создаем ренде-проход
    if (vkCreateRenderPass(vulkanLogicalDevice, &renderPassInfo, nullptr, &vulkanRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

// Создание пайплайна отрисовки
void createGraphicsPipeline() {
    // Уничтожаем старое
    if(vulkanPipeline != VK_NULL_HANDLE){
        vkDestroyPipeline(vulkanLogicalDevice, vulkanPipeline, nullptr);
        vulkanPipeline = VK_NULL_HANDLE;
    }
    if(vulkanPipelineLayout != VK_NULL_HANDLE){
        vkDestroyPipelineLayout(vulkanLogicalDevice, vulkanPipelineLayout, nullptr);
        vulkanPipelineLayout = VK_NULL_HANDLE;
    }
    
    // Читаем байт-код шейдеров
    std::vector<char> vertShaderCode = readFile("res/shaders/vert.spv");
    std::vector<char> fragShaderCode = readFile("res/shaders/frag.spv");
    
    // Создаем шейдерный модуль
    createShaderModule(vertShaderCode, vulkanVertexShader);
    createShaderModule(fragShaderCode, vulkanFragmentShader);
    
    // Описание настроек вершинного шейдера
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    memset(&vertShaderStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Вершинный шейдер
    vertShaderStageInfo.module = vulkanVertexShader;    // Модуль
    vertShaderStageInfo.pName = "main";     // Входная функция
    
    // Описание настроек фрагментного шейдера
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    memset(&fragShaderStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Фрагментный шейдер
    fragShaderStageInfo.module = vulkanFragmentShader;  // Модуль
    fragShaderStageInfo.pName = "main";     // Входная функция
    
    // Описание настроек глубины и трафарета у пайплайна
    // Поля depthBoundsTestEnable, minDepthBounds и maxDepthBounds используют для дополнительного теста связанной глубины.
    // По сути, это позволяет сохранить фрагменты, которые находятся в пределах заданного диапазона глубины, нам не понадобится.
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    memset(&depthStencil, 0, sizeof(VkPipelineDepthStencilStateCreateInfo));
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;     // Тест глубины включен
    depthStencil.depthWriteEnable = VK_TRUE;    // Запись глубины включена
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;   // Функция глубины
    depthStencil.depthBoundsTestEnable = VK_FALSE;  // TODO: ???
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Нужно для трафарета
    depthStencil.back = {};  // Нужно для трафарета
    
    // Описание вершин, шага по вершинам и описание данных
    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();
    
    // Описание формата входных данных
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    memset(&vertexInputInfo, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // Топология вершин
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    memset(&inputAssembly, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;   // Рисуем обычными треугольниками
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Настраиваем вьюпорт
    VkViewport viewport = {};
    memset(&viewport, 0, sizeof(VkViewport));
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkanSwapChainExtent.width;
    viewport.height = (float)vulkanSwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    // Выставляем сциссор
    VkRect2D scissor = {};
    memset(&scissor, 0, sizeof(VkRect2D));
    scissor.offset = {0, 0};
    scissor.extent = vulkanSwapChainExtent;
    
    // Создаем структуру настроек вьюпорта
    VkPipelineViewportStateCreateInfo viewportState = {};
    memset(&viewportState, 0, sizeof(VkPipelineViewportStateCreateInfo));
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // Настройки растеризатора
    //  - Если depthClampEnable установлен в значение VK_TRUE, тогда фрагменты, находящиеся за ближней и дальней плоскостью, прикрепляются к ним, а не отбрасываются.
    // Это бывает полезно в ряде случаев, например для карты теней. Для использования необходимо включить функцию GPU.
    // - Если rasterizerDiscardEnable установлен в значение VK_TRUE, тогда геометрия не проходит через растеризатор.
    // По сути это отключает любой вывод в фреймбуфер.
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    memset(&rasterizer, 0, sizeof(VkPipelineRasterizationStateCreateInfo));
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;         // Включено ли округление глубины крайними значениями
    rasterizer.rasterizerDiscardEnable = VK_FALSE;  // Графика рисуется в буффер кадра
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Заполненные полигоны
    rasterizer.lineWidth = 1.0f;                    // Толщина линии
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;    //  Задняя часть
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Обход по часовой стрелке для фронтальной стороны
    rasterizer.depthBiasEnable = VK_FALSE;          // Смещение по глубине отключено
    
    // Настройка антиаллиасинга с помощью мультисемплинга
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    memset(&multisampling, 0, sizeof(VkPipelineMultisampleStateCreateInfo));
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = APPLICATION_SAMPLING_VALUE;    // Кратность семплирования
    multisampling.minSampleShading = 1.0f;      // Optional
    multisampling.pSampleMask = nullptr;        // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional
    
    // Настройки классического блендинга
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    memset(&colorBlendAttachment, 0, sizeof(VkPipelineColorBlendAttachmentState));
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;    // Цвета, которые пишем
    colorBlendAttachment.blendEnable = VK_FALSE;    // Блендинг выключен
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    // Настройка конкретного блендинга
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    memset(&colorBlending, 0, sizeof(VkPipelineColorBlendStateCreateInfo));
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    // Лаяут пайплайна
    VkDescriptorSetLayout setLayouts[] = {vulkanDescriptorSetLayout};   // Лаяют для юниформ буффер и семплера
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    memset(&pipelineLayoutInfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts; // Устанавливаем лаяут
    pipelineLayoutInfo.pushConstantRangeCount = 0;  // TODO: Пуш-константы??
    pipelineLayoutInfo.pPushConstantRanges = 0;
    // Пуш константы нужны для того, чтобы передавать данные в отрисовку, как альтернатива юниформам, но без изменения??
    
    if (vkCreatePipelineLayout(vulkanLogicalDevice, &pipelineLayoutInfo, nullptr, &vulkanPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    
    // TODO: Наследование позволяет ускорить переключение пайплайнов с общим родителем
    
    // Непосредственно создание пайплайна
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    memset(&pipelineInfo, 0, sizeof(VkGraphicsPipelineCreateInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;            // 2 стадии, вершинный и фрагментный шейдер
    pipelineInfo.pStages = shaderStages;    // Шейдеры
    pipelineInfo.pVertexInputState = &vertexInputInfo;  // Описание входных вершин
    pipelineInfo.pInputAssemblyState = &inputAssembly;  // Топология вершин (Рисуем треугольниками)
    pipelineInfo.pViewportState = &viewportState;       // Вьюпорт и scissor отрисовки
    pipelineInfo.pRasterizationState = &rasterizer;     // Растеризатор (кулинг сторон и режим полигона)
    pipelineInfo.pDepthStencilState = &depthStencil;    // Настройки работы с глубиной
    pipelineInfo.pMultisampleState = &multisampling;    // Настройки семплирования для антиалиассинга
    pipelineInfo.pColorBlendState = &colorBlending;     // Настройка смешивания цветов
    pipelineInfo.layout = vulkanPipelineLayout;         // Лаяут пайплайна (Описание буфферов юниформов и семплеров)
    pipelineInfo.renderPass = vulkanRenderPass;         // Рендер-проход
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;   // Родительский пайплайн
    pipelineInfo.pDynamicState = nullptr;               // Динамическое состояние отрисовки
    
    if (vkCreateGraphicsPipelines(vulkanLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vulkanPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}


// Создаем фреймбуфферы для вьюшек изображений свопчейна
void createFramebuffers(){
    // Уничтожаем старые буфферы свопчейнов
    if (vulkanSwapChainFramebuffers.size() > 0) {
        for (const auto& buffer: vulkanSwapChainFramebuffers) {
            vkDestroyFramebuffer(vulkanLogicalDevice, buffer, nullptr);
        }
        vulkanSwapChainFramebuffers.clear();
    }
    
    // Ресайзим массив с фреймбуфферами свопчейна
    vulkanSwapChainFramebuffers.resize(vulkanSwapChainImageViews.size());
    
    // Для каждой вьюшки картинки создаем  фреймбуффер
    for (size_t i = 0; i < vulkanSwapChainImageViews.size(); i++) {
        // Список аттачментов
        std::array<VkImageView, 2> attachments = { {vulkanSwapChainImageViews[i], vulkanDepthImageView}};
        
        // Информация для создания фрейб-буфферов
        VkFramebufferCreateInfo framebufferInfo = {};
        memset(&framebufferInfo, 0, sizeof(VkFramebufferCreateInfo));
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vulkanRenderPass;  // Используем стандартный рендер-проход
        framebufferInfo.attachmentCount = attachments.size();   // Аттачменты
        framebufferInfo.pAttachments = attachments.data();      // Данные аттачментов
        framebufferInfo.width = vulkanSwapChainExtent.width;    // Размеры экрана
        framebufferInfo.height = vulkanSwapChainExtent.height;  // Размеры экрана
        framebufferInfo.layers = 1; // TODO: ???
        
        if (vkCreateFramebuffer(vulkanLogicalDevice, &framebufferInfo, nullptr, &(vulkanSwapChainFramebuffers[i])) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

// Создаем пулл комманд
void createCommandPool() {
    // Информация о пуле коммандных буфферов
    VkCommandPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkCommandPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vulkanRenderQueueFamilyIndex;   // Пулл будет для семейства очередей рендеринга
    poolInfo.flags = 0; // Optional
    
    if (vkCreateCommandPool(vulkanLogicalDevice, &poolInfo, nullptr, &vulkanCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

// Подбираем тип памяти буффера вершин
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Запрашиваем типы памяти физического устройства
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &memProperties);
    
    // Найдем тип памяти, который подходит для самого буфера
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("failed to find suitable memory type!");
}

// Создаем изображение
void createImage(uint32_t width, uint32_t height,
                 VkFormat format, VkImageTiling tiling, VkImageLayout layout, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage& image, VkDeviceMemory& imageMemory) {
    
    // Удаляем старые объекты
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(vulkanLogicalDevice, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vulkanLogicalDevice, imageMemory, nullptr);
        imageMemory = VK_NULL_HANDLE;
    }
    
    // Для поля initialLayout есть только два возможных значения:
    // VK_IMAGE_LAYOUT_UNDEFINED: Не и используется GPU и первое изменение (transition) отбросит все тексели.
    // VK_IMAGE_LAYOUT_PREINITIALIZED: Не и используется GPU, но первое изменение (transition) сохранит тексели.
    // Первый вариант подходит для изображений, которые будут использоваться в качестве вложений, как буфер цвета и глубины.
    // В таком случае не нужно заботиться о данных инициализации, т.к. они скорее всего будут очищены проходом рендера до начала использования. А если же вы хотите заполнить данные, такие как текстуры, тогда используйте второй вариант:
    
    // Информация об изображении
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D; // 2D текстура
    imageInfo.extent.width = width;     // Ширина
    imageInfo.extent.height = height;   // Высота
    imageInfo.extent.depth = 1;         // Глубина
    imageInfo.mipLevels = 1;            // Без мипмапов
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;          // Формат данных текстуры
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = layout; // Текстура заранее с нужными данными
    imageInfo.usage = usage;    // Флаги использования текстуры
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;  // Семплирование данной текстуры
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Режим междевайного доступа
    
    // Создаем изображение
    if (vkCreateImage(vulkanLogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    
    // Запрашиваем информацию о требованиях памяти для текстуры
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkanLogicalDevice, image, &memRequirements);
    
    // Подбираем нужный тип аллоцируемой памяти для требований и возможностей
    uint32_t memoryType = findMemoryType(memRequirements.memoryTypeBits, properties);
    VkMemoryAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkMemoryAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;    // Размер аллоцируемой памяти
    allocInfo.memoryTypeIndex = memoryType;             // Тип памяти
    
    if (vkAllocateMemory(vulkanLogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    
    // Цепляем к картинке буффер памяти
    vkBindImageMemory(vulkanLogicalDevice, image, imageMemory, 0);
}

// Запуск коммандного буффера на получение комманд
VkCommandBuffer beginSingleTimeCommands() {
    // Параметр level определяет, будет ли выделенный буфер команд первичным или вторичным буфером команд:
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Может быть передан очереди для исполнения, но не может быть вызван из других буферов команд.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: не может быть передан непосредственно, но может быть вызван из первичных буферов команд.
    VkCommandBufferAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkCommandBufferAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Первичный буффер, которыый будет исполняться сразу
    allocInfo.commandPool = vulkanCommandPool;      // Пул комманд
    allocInfo.commandBufferCount = 1;
    
    // Аллоцируем коммандный буффер для задач, который будут закидываться в очередь
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkanLogicalDevice, &allocInfo, &commandBuffer);
    
    // Параметр flags определяет, как использовать буфер команд. Возможны следующие значения:
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Буфер команд будет перезаписан сразу после первого выполнения.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: Это вторичный буфер команд, который будет в единственном render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
    
    // Настройки запуска коммандного буффера
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    // Запускаем буффер комманд
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

// Завершение коммандного буффера
void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // Заканчиваем прием комманд
    vkEndCommandBuffer(commandBuffer);
    
    // Структура с описанием отправки в буффер
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    // Отправляем задание на отрисовку в буффер отрисовки
    vkQueueSubmit(vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    
    // TODO: Ожидание передачи комманды в очередь на GPU???
    vkQueueWaitIdle(vulkanGraphicsQueue);
    /*std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
    vkQueueWaitIdle(vulkanGraphicsQueue);
    std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration elapsed = time2 - time1;
    int64_t elapsedMicroSec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    printf("Wait duration (vkQueueWaitIdle(vulkanGraphicsQueue)): %lldmicroSec\n", elapsedMicroSec);
    fflush(stdout);*/
    
    // Удаляем коммандный буффер
    vkFreeCommandBuffers(vulkanLogicalDevice, vulkanCommandPool, 1, &commandBuffer);
}

// Есть ли поддержка трафарета в формате
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Перевод изображения из одного лаяута в другой (из одного способа использования в другой)
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    // Создаем коммандный буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    // Создаем барьер памяти для картинок
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;  // Старый лаяут (способ использования)
    barrier.newLayout = newLayout;  // Новый лаяут (способ использования)
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.image = image;  // Изображение, которое меняется
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;    // Изображение использовалось для цвета
    barrier.subresourceRange.baseMipLevel = 0;  // 0 левел мипмапов
    barrier.subresourceRange.levelCount = 1;    // 1 уровень мипмапов
    barrier.subresourceRange.baseArrayLayer = 0;    // Базовый уровень
    barrier.subresourceRange.layerCount = 1;        // 1 уровень
    
    // Настраиваем условия ожиданий для конвертации
    // TODO: ???
    if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;       // Ожидание записи хостом данных
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;    // TODO: Дальнейшие действия возможнны после чтения из текстуры???
    } else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    
    // TODO: ???
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    // Закидываем в очередь барьер конвертации использования для изображения
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Закидываем на верх пайплайна
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Закидываем на верх пайплайна
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    endSingleTimeCommands(commandBuffer);
}

// Закидываем в очередь операцию копирования текстуры
void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {
    // TODO: Надо ли для группы операций с текстурами каждый раз создавать коммандный буффер?? Может быть можно все делать в одном?
    // Создаем коммандный буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    // Описание ресурса
    VkImageSubresourceLayers subResource = {};
    memset(&subResource, 0, sizeof(VkImageSubresourceLayers));
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Текстура с цветом
    subResource.layerCount = 1; // Всего 1н слой
    subResource.baseArrayLayer = 0; // 0й слой
    subResource.mipLevel = 0;   // 0й уровень мипмаппинга
    
    // Регион копирования текстуры
    VkImageCopy region = {};
    memset(&region, 0, sizeof(VkImageCopy));
    region.srcSubresource = subResource;
    region.dstSubresource = subResource;
    region.srcOffset = {0, 0, 0};
    region.dstOffset = {0, 0, 0};
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;
    
    // Создаем задачу на копирование данных
    vkCmdCopyImage(commandBuffer,
                   srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);
    
    // Завершаем буффер комманд
    endSingleTimeCommands(commandBuffer);
}

// Подбираем формат текстуры в зависимости от доступных на устройстве
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        // Запрашиваем информацию для формата
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vulkanPhysicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    throw std::runtime_error("failed to find supported format!");
}

// Подбираем нужный формат глубины
VkFormat findDepthFormat() {
    std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    vulkanDepthFormat = findSupportedFormat(candidates,
                                            VK_IMAGE_TILING_OPTIMAL,
                                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return vulkanDepthFormat;
}

// Создаем буфферы для глубины
void createDepthResources() {
    createImage(vulkanSwapChainExtent.width, vulkanSwapChainExtent.height,
                vulkanDepthFormat,                                  // Формат текстуры
                VK_IMAGE_TILING_OPTIMAL,                            // Оптимальный тайлинг
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,   // Используем сразу правильный лаяут для текстуры
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,        // Использоваться будет в качестве аттачмента глубины
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                // Хранится только на GPU
                vulkanDepthImage, vulkanDepthImageMemory);
    
    // Создаем вью для изображения буффера глубины
    createImageView(vulkanDepthImage, vulkanDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, vulkanDepthImageView);
    
    // Наверное нужно? Но и так работает
    //transitionImageLayout(vulkanDepthImage, vulkanDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

// Создание текстуры из изображения
void createTextureImage() {
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load("res/textures/chalet.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // STBI_rgb_alpha STBI_default
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    
    // Указатели на временную текстуру и целевую
    VkImage stagingImage = VK_NULL_HANDLE;
    VkDeviceMemory stagingImageMemory = VK_NULL_HANDLE;
    
    // VK_IMAGE_TILING_LINEAR - специально, для исходного изображения
    // http://vulkanapi.ru/2016/12/17/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-45/
    createImage(texWidth, texHeight,
                VK_FORMAT_R8G8B8A8_UNORM,           // Формат текстуры
                VK_IMAGE_TILING_LINEAR,             // Тайлинг
                VK_IMAGE_LAYOUT_PREINITIALIZED,     // Чтобы данные не уничтожились при первом использовании - используем PREINITIALIZED
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,    // Используется для передачи в другую текстуру данных
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Настраиваем работу с памятью так, чтобы было доступно на CPU
                stagingImage,
                stagingImageMemory);
    
    // Описание подресурса для изображения
    VkImageSubresource subresource = {};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel = 0;
    subresource.arrayLayer = 0;
    
    // Создание лаяута для подресурса
    VkSubresourceLayout stagingImageLayout;
    vkGetImageSubresourceLayout(vulkanLogicalDevice, stagingImage, &subresource, &stagingImageLayout);
    
    // Мапим данные текстуры в адресное пространство CPU
    void* data = nullptr;
    vkMapMemory(vulkanLogicalDevice, stagingImageMemory, 0, imageSize, 0, &data);
    
    // Копируем целиком или построчно в зависимости от размера и выравнивания на GPU
    if (stagingImageLayout.rowPitch == texWidth * 4) {
        memcpy(data, pixels, (size_t) imageSize);
    } else {
        uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);
        
        for (int y = 0; y < texHeight; y++) {
            memcpy(&dataBytes[y * stagingImageLayout.rowPitch],
                   &pixels[y * texWidth * 4],
                   texWidth * 4);
        }
    }
    
    // Размапим данные
    vkUnmapMemory(vulkanLogicalDevice, stagingImageMemory);
    
    // Создаем рабочее изображение для последующего использования
    createImage(texWidth, texHeight,
                VK_FORMAT_R8G8B8A8_UNORM,   // Формат картинки
                VK_IMAGE_TILING_OPTIMAL,    // тайлинг
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,   // Лаяут использования - принимает данные из другой текстуры
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,   // Используется как получаетель + для отрисовки
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // Хранится только на GPU
                vulkanTextureImage,
                vulkanTextureImageMemory);
    
    // Конвертирование исходной буфферной текстуры с данными в формат копирования на GPU
    transitionImageLayout(stagingImage,
                          VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_PREINITIALIZED,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    //transitionImageLayout(vulkanTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // Копируем данные в пределах GPU из временной текстуры в целевую
    copyImage(stagingImage, vulkanTextureImage, texWidth, texHeight);
    
    // Конвертируем использование текстуры в оптимальное для рендеринга
    transitionImageLayout(vulkanTextureImage,
                          VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Удаляем временные объекты
    vkDestroyImage(vulkanLogicalDevice, stagingImage, nullptr);
    vkFreeMemory(vulkanLogicalDevice, stagingImageMemory, nullptr);
    
    // Учищаем буффер данных картинки
    stbi_image_free(pixels);
}

// Создаем вью текстуры
void createTextureImageView() {
    createImageView(vulkanTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, vulkanTextureImageView);
}

// Создание семплера для картинки
void createTextureSampler() {
    // Описание семплирования для текстуры
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;   // Линейное
    samplerInfo.minFilter = VK_FILTER_LINEAR;   // Линейное
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   // Ограничение по границе
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   // Ограничение по границе
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   // Ограничение по границе
    samplerInfo.anisotropyEnable = VK_FALSE;    // Анизотропная фильтрация
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    // Создаем семплер
    if (vkCreateSampler(vulkanLogicalDevice, &samplerInfo, nullptr, &vulkanTextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

// Грузим данные для модели
void loadModel(){
    printf("Model loading started\n");
    fflush(stdout);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "res/models/chalet.obj")) {
        throw std::runtime_error(err);
    }
    
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            
            vertex.color = {1.0f, 1.0f, 1.0f};
            
            vulkanVertices.push_back(vertex);
            vulkanIndices.push_back(vulkanIndices.size());
        }
    }
    
    vulkanTotalVertexesCount = vulkanVertices.size();
    vulkanTotalIndexesCount = vulkanIndices.size();
    
    printf("Model loading complete\n");
    fflush(stdout);
}

// Создаем буффер нужного размера
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // Удаляем старое, если есть
    if(bufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(vulkanLogicalDevice, bufferMemory, nullptr);
        vulkanVertexBufferMemory = VK_NULL_HANDLE;
    }
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkanLogicalDevice, buffer, nullptr);
        vulkanVertexBuffer = VK_NULL_HANDLE;
    }
    
    // VK_SHARING_MODE_EXCLUSIVE: изображение принадлежит одному семейству в один момент времени и должно быть явно передано другому семейству. Данный вариант обеспечивает наилучшую производительность.
    // VK_SHARING_MODE_CONCURRENT: изображение может быть использовано несколькими семействами без явной передачи.
    
    // Описание формата буффера
    VkBufferCreateInfo bufferInfo = {};
    memset(&bufferInfo, 0, sizeof(VkBufferCreateInfo));
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;     // Размер буффера
    bufferInfo.usage = usage;   // Использование данного буффера
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // изображение принадлежит одному семейству в один момент времени и должно быть явно передано другому семейству. Данный вариант обеспечивает наилучшую производительность.
    
    // Непосредственно создание буффера
    if (vkCreateBuffer(vulkanLogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    
    // Запрашиваем данные о необходимой памяти
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanLogicalDevice, buffer, &memRequirements);
    
    // Настройки аллокации буффера
    uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkMemoryAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkMemoryAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    // Выделяем память для буффера
    if (vkAllocateMemory(vulkanLogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }
    
    // Подцепляем память к буфферу
    // Последний параметр – смещение в области памяти. Т.к. эта память выделяется специально для буфера вершин, смещение просто 0.
    // Если же оно не будет равно нулю, то значение должно быть кратным memRequirements.alignment.
    vkBindBufferMemory(vulkanLogicalDevice, buffer, bufferMemory, 0);
}

// Копирование буффера
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // Запускаем буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    // Ставим в очередь копирование буффера
    VkBufferCopy copyRegion = {};
    memset(&copyRegion, 0, sizeof(VkBufferCopy));
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    // Заканчиваем буффер
    endSingleTimeCommands(commandBuffer);
}

// Создание буфферов вершин
void createVertexBuffer(){
    // Размер буфферов
    VkDeviceSize bufferSize = sizeof(vulkanVertices[0]) * vulkanVertices.size();
    
    // Создание временного буффера для передачи данных
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Буффер может быть использован как источник данных для копирования
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,    // Хранится в оперативке CPU
                 stagingBuffer, stagingBufferMemory);
    
    // Маппим видео-память в адресное пространство оперативной памяти
    void* data = nullptr;
    vkMapMemory(vulkanLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    
    // Копируем вершины в память
    memcpy(data, vulkanVertices.data(), (size_t)bufferSize);
    
    // Размапим
    vkUnmapMemory(vulkanLogicalDevice, stagingBufferMemory);
    
    // Создаем рабочий буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // Буффер может принимать данные + для отрисовки используется
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится на видео-карте
                 vulkanVertexBuffer, vulkanVertexBufferMemory);
    
    // Ставим задачу на копирование буфферов
    copyBuffer(stagingBuffer, vulkanVertexBuffer, bufferSize);
    
    // Удаляем временный буффер, если есть
    if(stagingBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(vulkanLogicalDevice, stagingBufferMemory, nullptr);
        stagingBufferMemory = VK_NULL_HANDLE;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkanLogicalDevice, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    
    // Чистим исходные данные
    vulkanVertices.clear();
}

// Создание буффера индексов
void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(vulkanIndices[0]) * vulkanIndices.size();
    
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    
    // Создаем временный буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Буффер используется как исходник для копирования
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Видимый из CPU
                 stagingBuffer, stagingBufferMemory);
    
    // Маппим видео-память в адресное пространство оперативной памяти
    void* data = nullptr;
    vkMapMemory(vulkanLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    
    // Копируем данные
    memcpy(data, vulkanIndices.data(), (size_t)bufferSize);
    
    // Размапим память
    vkUnmapMemory(vulkanLogicalDevice, stagingBufferMemory);
    
    // Создаем рабочий буффер
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, // Используется как получатель + индексный буффер
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится только на GPU
                 vulkanIndexBuffer, vulkanIndexBufferMemory);
    
    // Ставим задачу на копирование буфферов
    copyBuffer(stagingBuffer, vulkanIndexBuffer, bufferSize);
    
    // Удаляем временный буффер, если есть
    if(stagingBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(vulkanLogicalDevice, stagingBufferMemory, nullptr);
        stagingBufferMemory = VK_NULL_HANDLE;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkanLogicalDevice, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    
    // Чистим исходные данные
    vulkanIndices.clear();
}

// Создаем буффер юниформов
void createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    // Буффер для юниформов для CPU
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Используется как исходник для передачи на отрисовку
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,    // Доступно для изменения на GPU
                 vulkanUniformStagingBuffer, vulkanUniformStagingBufferMemory);
    // Буффер для юниформов на GPU
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // Испольузется как получаетель + юниформ буффер
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится только на GPU
                 vulkanUniformBuffer, vulkanUniformBufferMemory);
}

// Создаем пул дескрипторов ресурсов
void createDescriptorPool() {
    // Структура с типами пулов
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    // Юниформ буффер
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    // Семплер для текстуры
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;
    
    // Создаем пул
    VkDescriptorPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(vulkanLogicalDevice, &poolInfo, nullptr, &vulkanDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

// Создаем набор дескрипторов ресурсов
void createDescriptorSet() {
    // Настройки аллокатора для дескрипторов ресурсов
    VkDescriptorSetLayout layouts[] = {vulkanDescriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vulkanDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;
    
    // Аллоцируем дескрипторы в пуле
    if (vkAllocateDescriptorSets(vulkanLogicalDevice, &allocInfo, &vulkanDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }
    
    // Описание дескриптора юниформа
    VkDescriptorBufferInfo bufferInfo = {};
    memset(&bufferInfo, 0, sizeof(VkDescriptorBufferInfo));
    bufferInfo.buffer = vulkanUniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
    
    // Описание дескриптора семплера
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vulkanTextureImageView;
    imageInfo.sampler = vulkanTextureSampler;
    
    // НАстройки дескрипторов
    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = vulkanDescriptorSet;   // Набор дескрипторов из пула
    descriptorWrites[0].dstBinding = 0;                 // Биндится на 0м значении в шейдере
    descriptorWrites[0].dstArrayElement = 0;            // 0 элемент
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Тип - юниформ буффер
    descriptorWrites[0].descriptorCount = 1;            // 1н дескриптор
    descriptorWrites[0].pBufferInfo = &bufferInfo;      // Описание буффера
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = vulkanDescriptorSet;   // Набор дескрипторов из пула
    descriptorWrites[1].dstBinding = 1;                 // Биндится на 1м значении в шейдере
    descriptorWrites[1].dstArrayElement = 0;            // 0 элемент
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // Тип - семплер
    descriptorWrites[1].descriptorCount = 1;            // 1н дескриптор
    descriptorWrites[1].pImageInfo = &imageInfo;        // Описание изображения
    
    // Обновляем описание дескрипторов на устройстве
    vkUpdateDescriptorSets(vulkanLogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

// Создаем коммандные буфферы
void createCommandBuffers() {

    // Очистка старых буфферов комманд
    if (vulkanCommandBuffers.size() > 0) {
        vkFreeCommandBuffers(vulkanLogicalDevice, vulkanCommandPool, vulkanCommandBuffers.size(), vulkanCommandBuffers.data());
        vulkanCommandBuffers.clear();
    }
    
    // Ресайзим массив
    vulkanCommandBuffers.resize(vulkanSwapChainFramebuffers.size());
    
    // Настройки создания коммандного буффера
    VkCommandBufferAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkCommandBufferAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkanCommandPool;  // Создаем в общем пуле комманд
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Первичный буффер
    allocInfo.commandBufferCount = (uint32_t)vulkanCommandBuffers.size();
    
    // Аллоцируем память под коммандные буфферы
    if (vkAllocateCommandBuffers(vulkanLogicalDevice, &allocInfo, vulkanCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    
    for (size_t i = 0; i < vulkanCommandBuffers.size(); i++) {
        // Параметр flags определяет, как использовать буфер команд. Возможны следующие значения:
        // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Буфер команд будет перезаписан сразу после первого выполнения.
        // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: Это вторичный буфер команд, который будет в единственном render pass.
        // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
        
        // Информация о запуске коммандного буффера
        VkCommandBufferBeginInfo beginInfo = {};
        memset(&beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.
        beginInfo.pInheritanceInfo = nullptr; // Optional
        
        // Запуск коммандного буффера
        vkBeginCommandBuffer(vulkanCommandBuffers[i], &beginInfo);
        
        // Информация о запуске рендер-прохода
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        VkRenderPassBeginInfo renderPassInfo = {};
        memset(&renderPassInfo, 0, sizeof(VkRenderPassBeginInfo));
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vulkanRenderPass;   // Рендер проход
        renderPassInfo.framebuffer = vulkanSwapChainFramebuffers[i];    // Фреймбуффер смены кадров
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vulkanSwapChainExtent;
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();
        
        // Запуск рендер-прохода
        // VK_SUBPASS_CONTENTS_INLINE: Команды render pass будут включены в первичный буфер команд и вторичные буферы команд не будут задействованы.
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: Команды render pass будут выполняться из вторичных буферов.
        vkCmdBeginRenderPass(vulkanCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // Устанавливаем пайплайн у коммандного буффера
        vkCmdBindPipeline(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline);
        
        // Привязываем вершинный буффер к пайлпайну
        VkBuffer vertexBuffers[] = {vulkanVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vulkanCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        
        // Привязываем индексный буффер к пайплайну
        vkCmdBindIndexBuffer(vulkanCommandBuffers[i], vulkanIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        // Подключаем дескрипторы ресурсов для юниформ буффера
        vkCmdBindDescriptorSets(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayout, 0, 1, &vulkanDescriptorSet, 0, nullptr);
        
        // Вызов отрисовки - 3 вершины, 1 инстанс, начинаем с 0 вершины и 0 инстанса
        // vkCmdDraw(vulkanCommandBuffers[i], QUAD_VERTEXES.size(), 1, 0, 0);
        // Вызов поиндексной отрисовки - индексы вершин, один инстанс
        vkCmdDrawIndexed(vulkanCommandBuffers[i], vulkanTotalIndexesCount, 1, 0, 0, 0);
        
        // Заканчиваем рендер проход
        vkCmdEndRenderPass(vulkanCommandBuffers[i]);
        
        // Заканчиваем подготовку коммандного буффера
        if (vkEndCommandBuffer(vulkanCommandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

// Создаем семафоры для синхронизаций, чтобы не начинался энкодинг, пока не отобразится один из старых кадров
void createSemaphores(){
    VkSemaphoreCreateInfo semaphoreInfo = {};
    memset(&semaphoreInfo, 0, sizeof(VkSemaphoreCreateInfo));
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    // Создаем семафор для отображения и для кодирования графики
    if (vkCreateSemaphore(vulkanLogicalDevice, &semaphoreInfo, nullptr, &vulkanImageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vulkanLogicalDevice, &semaphoreInfo, nullptr, &vulkanRenderFinishedSemaphore) != VK_SUCCESS) {
        
        throw std::runtime_error("failed to create semaphores!");
    }
}

// Вызывается при различных ресайзах окна
void recreateSwapChain() {
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(vulkanGraphicsQueue);
    vkQueueWaitIdle(vulkanPresentQueue);
    vkDeviceWaitIdle(vulkanLogicalDevice);
    
    // Заново пересоздаем свопчейны, старые удалятся внутри
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
}

// Обновляем юниформ буффер
void updateUniformBuffer(float delta){
    rotateAngle += delta * 30.0f;
    
    UniformBufferObject ubo = {};
    memset(&ubo, 0, sizeof(UniformBufferObject));
    ubo.model = glm::rotate(glm::mat4(), glm::radians(rotateAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapChainExtent.width / (float)vulkanSwapChainExtent.height, 0.1f, 10.0f);
    
    // GLM был разработан для OpenGL, где координата Y клип координат перевернута,
    // самым простым путем решения данного вопроса будет изменить знак оси Y в матрице проекции
    //ubo.proj[1][1] *= -1;
    
    void* data = nullptr;
    vkMapMemory(vulkanLogicalDevice, vulkanUniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vulkanLogicalDevice, vulkanUniformStagingBufferMemory);
    
    // Закидываем задачу на копирование буффера
    copyBuffer(vulkanUniformStagingBuffer, vulkanUniformBuffer, sizeof(ubo));
}

// Непосредственно отрисовка кадра
void drawFrame() {
    // Запрашиваем изображение для отображения из swapchain, время ожидания делаем максимальным
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(vulkanLogicalDevice, vulkanSwapchain,
                                            std::numeric_limits<uint64_t>::max(),
                                            vulkanImageAvailableSemaphore, // Семафор ожидания доступной картинки
                                            VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    // Настраиваем отправление в очередь комманд отрисовки
    // http://vulkanapi.ru/2016/11/14/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-29-%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D0%B8%D0%BD%D0%B3-%D0%B8-%D0%BF%D1%80%D0%B5%D0%B4%D1%81%D1%82%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5-hello-wo/
    VkSemaphore waitSemaphores[] = {vulkanImageAvailableSemaphore}; // Семафор ожидания картинки для вывода туда графики
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};    // Ждать будем возможности вывода в буфер цвета
    VkSemaphore signalSemaphores[] = {vulkanRenderFinishedSemaphore}; // Семафор оповещения о завершении рендеринга
    VkSubmitInfo submitInfo = {};
    memset(&submitInfo, 0, sizeof(VkSubmitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;    // Ожидаем доступное изображение, в которое можно было бы записывать пиксели
    submitInfo.pWaitDstStageMask = waitStages;      // Ждать будем возможности вывода в буфер цвета
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanCommandBuffers[imageIndex]; // Указываем коммандный буффер отрисовки
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    // Кидаем в очередь задачу на отрисовку с указанным коммандным буффером
    if (vkQueueSubmit(vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Настраиваем задачу отображения полученного изображения
    VkSwapchainKHR swapChains[] = {vulkanSwapchain};
    VkPresentInfoKHR presentInfo = {};
    memset(&presentInfo, 0, sizeof(VkPresentInfoKHR));
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Ожидаем окончания подготовки кадра с помощью семафора
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    // Закидываем в очередь задачу отображения картинки
    VkResult presentResult = vkQueuePresentKHR(vulkanPresentQueue, &presentInfo);
    
    // В случае проблем - пересоздаем свопчейн
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

// Коллбек, вызываемый при изменении размеров окна приложения
void onGLFWWindowResized(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    recreateSwapChain();
}

#ifndef _MSVC_LANG
int main(int argc, char** argv) {
#else
int local_main(int argc, char** argv) {
#endif
    glfwInit();
    
    // Говорим GLFW, что не нужно создавать GL контекст
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Окно без изменения размера
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Создаем окно
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onGLFWWindowResized);
    
    // Проверяем наличие поддержки Vulkan
    int vulkanSupportStatus = glfwVulkanSupported();
    if (vulkanSupportStatus != GLFW_TRUE){
        printf("Vulkan support not found, error 0x%08x\n", vulkanSupportStatus);
		fflush(stdout);
        throw std::runtime_error("Vulkan support not found!");
    }
    
    // Список доступных слоев валидации
    std::vector<VkLayerProperties> allValidationLayers = getAllValidationLayers();
    for(const VkLayerProperties& layerInfo: allValidationLayers){
        printf("Validation layer available: %s\n", layerInfo.layerName);
        fflush(stdout);
    }
    
    // Создание инстанса Vulkan
    createVulkanInstance();
    
    // Настраиваем коллбек
    setupDebugCallback();
    
    // Создаем плоскость отрисовки
    createDrawSurface();
    
    // Получаем GPU устройство
    pickPhysicalDevice();
    
    // Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
    createLogicalDeviceAndQueue();
    
    // Создание логики смены кадров
    createSwapChain();
    
    // Создание вьюшек изображений буффера кадра
    createImageViews();
    
    // Ищем формат буффера глубины
    findDepthFormat();
    
    // Создание рендер-прохода
    createRenderPass();
    
    // Создаем дескриптор для буффера юниформов
    createDescriptorSetLayout();
    
    // Создание пайплайна отрисовки
    createGraphicsPipeline();
    
    // Создаем пулл комманд
    createCommandPool();

    // Создаем ресурсы для глубины
    createDepthResources();
    
    // Создаем фреймбуфферы
    createFramebuffers();
    
    // Создание текстуры из изображения
    createTextureImage();
    
    // Создание вью для текстуры
    createTextureImageView();
    
    // Создаем семплер для картинки
    createTextureSampler();
    
    // Грузим нашу модель
    loadModel();
    
    // Создание буфферов вершин
    createVertexBuffer();
    
    // Создание индексного буффера
    createIndexBuffer();
    
    // Создаем буффер юниформов
    createUniformBuffer();
    
    // Создаем пул дескрипторов ресурсов
    createDescriptorPool();
    
    // Создаем набор дескрипторов
    createDescriptorSet();
    
    // Создаем коммандные буфферы
    createCommandBuffers();
    
    // Создаем семафоры
    createSemaphores();
    
    // Цикл обработки графики
    std::chrono::high_resolution_clock::time_point lastDrawTime = std::chrono::high_resolution_clock::now();
    double lastFrameDuration = 1.0/60.0;
    int totalFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Обновляем юниформы
        updateUniformBuffer(lastFrameDuration);
        
        // Непосредственно отрисовка кадра
        drawFrame();
        
        // Стабилизация времени кадра
        std::chrono::high_resolution_clock::duration curFrameDuration = std::chrono::high_resolution_clock::now() - lastDrawTime;
        std::chrono::high_resolution_clock::duration sleepDuration = std::chrono::milliseconds(static_cast<int>(1.0/60.0 * 1000.0)) - curFrameDuration;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count() > 0) {
            std::this_thread::sleep_for(sleepDuration);
        }
        // Расчет времени кадра
        lastFrameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastDrawTime).count() / 1000.0;
        lastDrawTime = std::chrono::high_resolution_clock::now(); // TODO: Возможно - правильнее было бы перетащить в начало цикла
        
        // FPS
        totalFrames++;
        if (totalFrames > 60) {
            totalFrames = 0;
            char outText[64];
            sprintf(outText, "Possible FPS: %d, sleep duration: %lldms", static_cast<int>(1.0/lastFrameDuration), std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count());
            glfwSetWindowTitle(window, outText);
        }
   }
    
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(vulkanGraphicsQueue);
    vkQueueWaitIdle(vulkanPresentQueue);
    vkDeviceWaitIdle(vulkanLogicalDevice);
    
    // Очищаем Vulkan
    // Удаляем старые объекты
    vkDestroyImage(vulkanLogicalDevice, vulkanDepthImage, nullptr);
    vkFreeMemory(vulkanLogicalDevice, vulkanDepthImageMemory, nullptr);
    vkDestroyImageView(vulkanLogicalDevice, vulkanDepthImageView, nullptr);
    vkDestroySampler(vulkanLogicalDevice, vulkanTextureSampler, nullptr);
    vkDestroyImageView(vulkanLogicalDevice, vulkanTextureImageView, nullptr);
    vkDestroyImage(vulkanLogicalDevice, vulkanTextureImage, nullptr);
    vkFreeMemory(vulkanLogicalDevice, vulkanTextureImageMemory, nullptr);
    vkDestroyDescriptorPool(vulkanLogicalDevice, vulkanDescriptorPool, nullptr);
    vkDestroyBuffer(vulkanLogicalDevice, vulkanUniformStagingBuffer, nullptr);
    vkFreeMemory(vulkanLogicalDevice, vulkanUniformStagingBufferMemory, nullptr);
    vkDestroyBuffer(vulkanLogicalDevice, vulkanUniformBuffer, nullptr);
    vkFreeMemory(vulkanLogicalDevice, vulkanUniformBufferMemory, nullptr);
    vkFreeMemory(vulkanLogicalDevice, vulkanIndexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanLogicalDevice, vulkanIndexBuffer, nullptr);
    vkFreeMemory(vulkanLogicalDevice, vulkanVertexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanLogicalDevice, vulkanVertexBuffer, nullptr);
    vkDestroySemaphore(vulkanLogicalDevice, vulkanImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(vulkanLogicalDevice, vulkanRenderFinishedSemaphore, nullptr);
    vkDestroyCommandPool(vulkanLogicalDevice, vulkanCommandPool, nullptr);
    for (const auto& buffer: vulkanSwapChainFramebuffers) {
        vkDestroyFramebuffer(vulkanLogicalDevice, buffer, nullptr);
    }
    vkDestroyPipeline(vulkanLogicalDevice, vulkanPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanLogicalDevice, vulkanPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(vulkanLogicalDevice, vulkanDescriptorSetLayout, nullptr);
    vkDestroyShaderModule(vulkanLogicalDevice, vulkanVertexShader, nullptr);
    vkDestroyShaderModule(vulkanLogicalDevice, vulkanFragmentShader, nullptr);
    vkDestroyRenderPass(vulkanLogicalDevice, vulkanRenderPass, nullptr);
    for(const auto& imageView: vulkanSwapChainImageViews){
        vkDestroyImageView(vulkanLogicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(vulkanLogicalDevice, vulkanSwapchain, nullptr);
    vkDestroyDevice(vulkanLogicalDevice, nullptr);
    vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, nullptr);
    #ifdef VALIDATION_LAYERS_ENABLED
        destroyDebugReportCallbackEXT(vulkanInstance, vulkanDebugCallback, nullptr);
    #endif
    vkDestroyInstance(vulkanInstance, nullptr);
    
    // Очищаем GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}

#ifdef _MSVC_LANG
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	local_main(0, NULL);
	return 0;
}
#endif

