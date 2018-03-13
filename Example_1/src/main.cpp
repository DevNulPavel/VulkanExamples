#include "CommonIncludes.h"
#include "CommonDefines.h"
#include "CommonConstants.h"


GLFWwindow* window = nullptr;
VkInstance vulkanInstance = VK_NULL_HANDLE;
VkDebugReportCallbackEXT vulkanDebugCallback = VK_NULL_HANDLE;
VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
int vulkanQueueFamilyIndex = 0;
VkDevice vulkanLogicalDevice = VK_NULL_HANDLE;
VkQueue vulcanGraphicsQueue = VK_NULL_HANDLE;


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
        
        if (vkCreateDebugReportCallbackEXT(vulkanInstance, &createInfo, nullptr, &vulkanDebugCallback) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug callback!");
        }
    #endif
}

// Для данного устройства ищем очереди отрисовки
int findQueueFamiliesIndexInDevice(VkPhysicalDevice device) {
    // Запрашиваем количество возможных типов очередей в устройстве
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    // Запрашиваем список очередей устройства
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily: queueFamilies) {
        // Для группы очередей проверяем, что там есть очереди + есть очередь отрисовки
        if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            return i;
        }
        i++;
    }
    
    return -1;
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
    std::map<int, std::pair<VkPhysicalDevice, int>> candidates;
    
    // Перебираем GPU на предмет производительности
    for (const auto& device: devices) {
        // Получаем индекс группы очередй отрисовки
        int renderQueueFamilyIndex = findQueueFamiliesIndexInDevice(device);
        if (renderQueueFamilyIndex >= 0) {
            // Оцениваем возможности устройства
            int score = rateDeviceScore(device);
            candidates[score] = std::pair<VkPhysicalDevice,int>(device, renderQueueFamilyIndex);
        }
    }
    
    // Есть ли вообще карты?
    if (candidates.size() == 0) {
        throw std::runtime_error("No picked GPU physical devices!");
    }
    
    // Получаем наилучший вариант GPU
    if (candidates.begin()->first > 0) {
        vulkanPhysicalDevice = candidates.begin()->second.first;
        vulkanQueueFamilyIndex = candidates.begin()->second.second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

// Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
void createLogicalDeviceAndQueue() {
    // Настройки создания очереди
    VkDeviceQueueCreateInfo queueCreateInfo;
    memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = vulkanQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    
    // Приоритет очереди
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    // Нужные фичи устройства (ничего не указываем)
    VkPhysicalDeviceFeatures deviceFeatures = {};
    memset(&deviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
    
    // Конфиг создания девайса
    VkDeviceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;
    
    #ifdef VALIDATION_LAYERS_ENABLED
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    #else
        createInfo.enabledLayerCount = 0;
    #endif
    
    // Пробуем создать логический девайс
    VkResult createStatus = vkCreateDevice(vulkanPhysicalDevice, &createInfo, nullptr, &vulkanLogicalDevice);
    if (createStatus != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }
    
    // Получаем очередь из девайса
    vkGetDeviceQueue(vulkanLogicalDevice, vulkanQueueFamilyIndex, 0, &vulcanGraphicsQueue);
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
    
    // Получаем GPU устройство
    pickPhysicalDevice();
    
    // Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
    createLogicalDeviceAndQueue();
    
    // Цикл обработки графики
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    
    // Очищаем Vulkan
    vkDestroyDevice(vulkanLogicalDevice, nullptr);
    #ifdef VALIDATION_LAYERS_ENABLED
        vkDestroyDebugReportCallbackEXT(vulkanInstance, vulkanDebugCallback, nullptr);
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

