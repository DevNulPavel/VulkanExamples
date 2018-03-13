#include "CommonIncludes.h"
#include "CommonDefines.h"
#include "CommonConstants.h"


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
VkFormat vulkanSwapChainImageFormat;
VkExtent2D vulkanSwapChainExtent;
std::vector<VkImageView> vulkanSwapChainImageViews;
std::vector<VkFramebuffer> vulkanSwapChainFramebuffers; // TODO: Deleter
VkShaderModule vulkanVertexShader = VK_NULL_HANDLE;
VkShaderModule vulkanFragmentShader = VK_NULL_HANDLE;
VkRenderPass vulkanRenderPass = VK_NULL_HANDLE; // TODO: Deleter
VkPipelineLayout vulkanPipelineLayout = VK_NULL_HANDLE;



struct FamiliesQueueIndexes {
    int renderQueueFamilyIndex;
    int presentQueueFamilyIndex;
    
    FamiliesQueueIndexes(){
        renderQueueFamilyIndex = -1;
        presentQueueFamilyIndex = -1;
    }
    bool isComplete(){
        return (renderQueueFamilyIndex >= 0) && (presentQueueFamilyIndex >= 0);
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    
    file.close();
    return buffer;
}

// К методам расширениям может не быть прямого доступа, поэтому создаем коллбек вручную
VkResult createDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Убираем коллбек
void destroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

// Получаем расширения Vulkan
std::vector<const char*> getRequiredExtensions() {
    std::vector<const char*> extensions;
    
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
        printf("Available extention name: %s\n", glfwExtensions[i]);
    }
    
    #ifdef VALIDATION_LAYERS_ENABLED
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    #endif
    
    return extensions;
}

// Проверка наличия уровней валидации
bool checkValidationLayerSupport() {
    // Количество уровней валидации
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    // Получаем слои валидации
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    // Проверяем, что запрошенные слои валидации доступны
    for(int i = 0; i < VALIDATION_LAYERS_COUNT; i++) {
        const char* layerName = VALIATION_LAYERS[i];
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
    
    return true;
}

// Создание инстанса Vulkan
void createVulkanInstance(){
    #ifdef VALIDATION_LAYERS_ENABLED
        if (!checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
    #endif
    
    // Структура с настройками приложения Vulkan
    VkApplicationInfo appInfo;
    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;  // Указываем используемую версию Vulkan
    
    // Получаем список расширений
    std::vector<const char*> extensions = getRequiredExtensions();
    
    // Структура настроек создания инстанса
    VkInstanceCreateInfo createInfo;
    memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());  // Включаем расширения
    createInfo.ppEnabledExtensionNames = extensions.data();
    #ifdef VALIDATION_LAYERS_ENABLED
        // Включаем стандартные слои валидации
        createInfo.enabledLayerCount = VALIDATION_LAYERS_COUNT;
        createInfo.ppEnabledLayerNames = VALIATION_LAYERS;
    #else
        // Не испольузем валидации
        createInfo.enabledLayerCount = 0;
    #endif
    
    // Непосредственно создание инстанса Vulkan
    VkResult createStatus = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
    if (createStatus != VK_SUCCESS) {
        printf("Failed to create instance! Status = %d\n", static_cast<int>(createStatus));
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
    return VK_FALSE;
}
#endif

// Устанавливаем коллбек для отладки
void setupDebugCallback() {
    #ifdef VALIDATION_LAYERS_ENABLED
        // Структура с описанием коллбека
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT; // Выводим только ошибки и варнинги
        createInfo.pfnCallback = debugCallback;
    
        if (createDebugReportCallbackEXT(vulkanInstance, &createInfo, nullptr, &vulkanDebugCallback) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug callback!");
        }
    #endif
}

// Создаем плоскость отрисовки
void createDrawSurface() {
    if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &vulkanSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

// Для данного устройства ищем очереди отрисовки
FamiliesQueueIndexes findQueueFamiliesIndexInDevice(VkPhysicalDevice device) {
    // Запрашиваем количество возможных типов очередей в устройстве
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    // Запрашиваем список очередей устройства
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    FamiliesQueueIndexes result;
    
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

// Оценка производительности и пригодности конкретной GPU
int rateDeviceScore(VkPhysicalDevice device) {
    // Получаем свойства и фичи устройства
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
    printf("Test GPU with name: %s\n", deviceProperties.deviceName);
    
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

// Проверяем, поддерживает ли девайс цепочку свопинга
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // Получаем количество расширений
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    // Получаем расширения
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    // Список требуемых расширений - смена кадров
    std::set<std::string> requiredExtensions(DEVICE_EXTENTIONS, DEVICE_EXTENTIONS + DEVICE_EXTENTIONS_COUNT);
    
    for (const auto& extension : availableExtensions) {
        printf("Available extention: %s\n", extension.extensionName);
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

// Запрашиваем поддержку свопа
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
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
    
    // Запрашиваем поддерживаемые форматы отображения
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
    
    // Иначе - перебираем список
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
    
    // Перебираем GPU на предмет производительности
    for (const VkPhysicalDevice& device: devices) {
        // Смотрим - есть ли у данного устройства поддержка свопа кадров в виде расширения?
        bool swapchainExtentionSupported = checkDeviceExtensionSupport(device);
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
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Нужные фичи устройства (ничего не указываем)
    VkPhysicalDeviceFeatures deviceFeatures;
    memset(&deviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
    
    // Конфиг создания девайса
    VkDeviceCreateInfo createInfo;
    memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = DEVICE_EXTENTIONS_COUNT;
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENTIONS;
    #ifdef VALIDATION_LAYERS_ENABLED
        createInfo.enabledLayerCount = VALIDATION_LAYERS_COUNT;
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
    #else
        createInfo.enabledLayerCount = 0;
    #endif
    
    // Пробуем создать логический девайс
    VkResult createStatus = vkCreateDevice(vulkanPhysicalDevice, &createInfo, nullptr, &vulkanLogicalDevice);
    if (createStatus != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }
    
    // Получаем очередь из девайса
    vkGetDeviceQueue(vulkanLogicalDevice, vulkanRenderQueueFamilyIndex, 0, &vulkanGraphicsQueue);
    vkGetDeviceQueue(vulkanLogicalDevice, vulkanPresentQueueFamilyIndex, 0, &vulkanPresentQueue);
}

// Создание логики смены кадров
void createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulkanPhysicalDevice);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vulkanSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    uint32_t queueFamilyIndices[] = {(uint32_t)vulkanRenderQueueFamilyIndex, (uint32_t)vulkanPresentQueueFamilyIndex};
    if (vulkanRenderQueueFamilyIndex != vulkanPresentQueueFamilyIndex) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    VkResult createStatus = vkCreateSwapchainKHR(vulkanLogicalDevice, &createInfo, nullptr, &vulkanSwapchain);
    if (createStatus != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    uint32_t imagesCount = 0;
    vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapchain, &imagesCount, nullptr);
    vulkanSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapchain, &imagesCount, vulkanSwapChainImages.data());
    
    vulkanSwapChainImageFormat = surfaceFormat.format;
    vulkanSwapChainExtent = extent;
}

// Создание вьюшек изображений буффера кадра
void createImageViews() {
    vulkanSwapChainImageViews.resize(vulkanSwapChainImages.size());
    for (uint32_t i = 0; i < vulkanSwapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vulkanSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(vulkanLogicalDevice, &createInfo, nullptr, &(vulkanSwapChainImageViews[i])) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void createShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = (uint32_t*)code.data();
    
    if (vkCreateShaderModule(vulkanLogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
}

// Создание пайплайна отрисовки
void createGraphicsPipeline() {
    auto vertShaderCode = readFile("res/shaders/vert.spv");
    auto fragShaderCode = readFile("res/shaders/frag.spv");
    
    createShaderModule(vertShaderCode, vulkanVertexShader);
    createShaderModule(fragShaderCode, vulkanFragmentShader);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vulkanVertexShader;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = vulkanFragmentShader;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkanSwapChainExtent.width;
    viewport.height = (float)vulkanSwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vulkanSwapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    
    if (vkCreatePipelineLayout(vulkanLogicalDevice, &pipelineLayoutInfo, nullptr, &vulkanPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

int main(int argc, char** argv) {
    glfwInit();
    
    // Говорим GLFW, что не нужно создавать GL контекст
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Окно без изменения размера
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    // Создаем окно
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    
    // Проверяем наличие поддержки Vulkan
    int vulkanSupportStatus = glfwVulkanSupported();
    if (vulkanSupportStatus != GLFW_TRUE){
        printf("Vulkan support not found, error 0x%08x\n", vulkanSupportStatus);
        throw std::runtime_error("Vulkan support not found!");
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
    
    // Создание пайплайна отрисовки
    createGraphicsPipeline();
    
    // Цикл обработки графики
    std::chrono::high_resolution_clock::time_point lastDrawTime = std::chrono::high_resolution_clock::now();
    double lastFrameDuration = 1.0/60.0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Стабилизация времени кадра
        std::chrono::high_resolution_clock::duration curFrameDuration = std::chrono::high_resolution_clock::now() - lastDrawTime;
        std::chrono::high_resolution_clock::duration sleepDuration = std::chrono::milliseconds(static_cast<int>(1.0/60.0 * 1000.0)) - curFrameDuration;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count() > 0) {
            std::this_thread::sleep_for(sleepDuration);
        }
        // Расчет времени кадра
        lastFrameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastDrawTime).count() / 1000.0;
        lastDrawTime = std::chrono::high_resolution_clock::now(); // TODO: Возможно - правильнее было бы перетащить в начало цикла
    }
    
    // Очищаем Vulkan
    vkDestroyPipelineLayout(vulkanLogicalDevice, vulkanPipelineLayout, nullptr);
    vkDestroyShaderModule(vulkanLogicalDevice, vulkanVertexShader, nullptr);
    vkDestroyShaderModule(vulkanLogicalDevice, vulkanFragmentShader, nullptr);
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

/*INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	printf("Windows start");
	__main(1, NULL);
	fflush(stdout);
	if (glfwVulkanSupported()) {
		printf("Vulkan available!!!");
		MessageBox(NULL, lpCmdLine, "Vulkan available!!!", 0);
	}
	MessageBox(NULL, lpCmdLine, "WinMain Demo", 0);
	return 0;
}*/

