#include "VulkanPhysicalDevice.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>
#include "CommonConstants.h"
#include "VulkanRender.h"


VulkanPhysicalDevice::VulkanPhysicalDevice():
    physicalDevice(VK_NULL_HANDLE){
        
    pickPhysicalDevice();
}

VulkanPhysicalDevice::~VulkanPhysicalDevice(){
    
}

VulkanQueuesFamiliesIndexes VulkanPhysicalDevice::getQueuesFamiliesIndexes(){
    return _queuesFamiliesIndexes;
}

VulkanSwapChainSupportDetails VulkanPhysicalDevice::getSwapChainSupportDetails(){
    return _swapchainSuppportDetails;
}

// Проверяем, поддерживает ли девайс цепочку свопинга
bool VulkanPhysicalDevice::checkDeviceRequiredExtensionSupport(VkPhysicalDevice device) {
    // Получаем количество расширений
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    // Получаем расширения
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    // Список требуемых расширений - смена кадров
    std::set<std::string> requiredExtensions(RenderI->vulkanInstance->instanceExtensions.begin(), RenderI->vulkanInstance->instanceExtensions.end());
    
    // Пытаемся убрать из списка требуемых расширений возможные
    for (const auto& extension : availableExtensions) {
        printf("Available extention at logical device: %s\n", extension.extensionName);
        fflush(stdout);
        requiredExtensions.erase(extension.extensionName);
    }
    
    // Если пусто, значит поддерживается
    return requiredExtensions.empty();
}

// Оценка производительности и пригодности конкретной GPU
int VulkanPhysicalDevice::rateDeviceScore(VkPhysicalDevice device) {
    // Свойства физического устройства
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    
    // Фичи физического устройства
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
    printf("Test GPU with name: %s, API version: %d.%d.%d,\n",
           deviceProperties.deviceName,
           VK_VERSION_MAJOR(deviceProperties.apiVersion),
           VK_VERSION_MINOR(deviceProperties.apiVersion),
           VK_VERSION_PATCH(deviceProperties.apiVersion));
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
VulkanSwapChainSupportDetails VulkanPhysicalDevice::querySwapChainSupport(VkPhysicalDevice device) {
    // Получаем возможности
    VulkanSwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, RenderI->vulkanWindowSurface->surface, &details.capabilities);
    
    // Запрашиваем поддерживаемые форматы буффера цвета
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, RenderI->vulkanWindowSurface->surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, RenderI->vulkanWindowSurface->surface, &formatCount, details.formats.data());
    }
    
    // Запрашиваем поддерживаемые типы отображения кадра
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, RenderI->vulkanWindowSurface->surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, RenderI->vulkanWindowSurface->surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

// Для данного устройства ищем очереди отрисовки
VulkanQueuesFamiliesIndexes VulkanPhysicalDevice::findQueueFamiliesIndexInDevice(VkPhysicalDevice device) {
    // Запрашиваем количество возможных семейств очередей в устройстве
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    // Запрашиваем список семейств очередей устройства
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    VulkanQueuesFamiliesIndexes result;
    
    // Подбираем информацию об очередях
    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily: queueFamilies) {
        
        // Для группы очередей отрисовки проверяем, что там есть очереди + есть очередь отрисовки
        if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            result.renderQueueFamilyIndex = i;
            result.renderQueueFamilyQueuesCount = queueFamily.queueCount;
        }
        
        // Провеяем, может является ли данная очередь - очередью отображения
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, RenderI->vulkanWindowSurface->surface, &presentSupport);
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

// Дергаем видеокарту
void VulkanPhysicalDevice::pickPhysicalDevice() {
    // Получаем количество GPU
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(RenderI->vulkanInstance->instance, &deviceCount, nullptr);
    
    // Есть ли вообще карты?
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    
    // Получаем список устройств
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(RenderI->vulkanInstance->instance, &deviceCount, devices.data());
    
    // Используем Map для автоматической сортировки по производительности
    std::map<int, std::tuple<VkPhysicalDevice, VulkanQueuesFamiliesIndexes, VulkanSwapChainSupportDetails>> candidates;
    
    // Перебираем GPU для поиска подходящего устройства
    for (const VkPhysicalDevice& device: devices) {
        // Смотрим - есть ли у данного устройства поддержка свопа кадров в виде расширения?
        bool swapchainExtentionSupported = checkDeviceRequiredExtensionSupport(device);
        if(swapchainExtentionSupported == false){
            continue;
        }
        
        // Проверяем, поддержку свопчейна у девайса, есть ли форматы и режимы отображения
        VulkanSwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        bool swapChainValid = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if (swapChainValid == false) {
            continue;
        }
        
        // Получаем индекс группы очередй отрисовки
        VulkanQueuesFamiliesIndexes familiesFound = findQueueFamiliesIndexInDevice(device);
        if (familiesFound.isComplete()) {
            // Оцениваем возможности устройства
            int score = rateDeviceScore(device);
            candidates[score] = std::tuple<VkPhysicalDevice, VulkanQueuesFamiliesIndexes, VulkanSwapChainSupportDetails>(device, familiesFound, swapChainSupport);
        }
    }
    
    // Есть ли вообще карты?
    if (candidates.size() == 0) {
        throw std::runtime_error("No picked GPU physical devices!");
    }
    
    // Получаем наилучший вариант GPU
    if (candidates.begin()->first > 0) {
        physicalDevice = std::get<0>(candidates.begin()->second);
        _queuesFamiliesIndexes = std::get<1>(candidates.begin()->second);
        _swapchainSuppportDetails = std::get<2>(candidates.begin()->second);
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}
