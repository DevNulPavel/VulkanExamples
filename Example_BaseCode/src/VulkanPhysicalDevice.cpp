#include "VulkanPhysicalDevice.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>
#include "Helpers.h"


VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstancePtr instance, 
										   const std::vector<const char*>& extensions, 
										   const std::vector<const char*>& layers, 
										   VulkanSurfacePtr surface):

    _vulkanInstance(instance),
    _vulkanExtensions(extensions),
	_vulkanLayers(layers),
    _vulkanSurface(surface),
    _device(VK_NULL_HANDLE){
        
    // Получаем количество GPU
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_vulkanInstance->getInstance(), &deviceCount, nullptr);
    
    // Есть ли вообще карты?
    if (deviceCount == 0) {
        LOG("Failed to find GPUs with Vulkan support!");
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    
    // Получаем список устройств
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_vulkanInstance->getInstance(), &deviceCount, devices.data());
    
    // Используем Map для автоматической сортировки по производительности
    std::map<int, std::tuple<VkPhysicalDevice, VulkanQueuesFamiliesIndexes, VulkanSwapChainSupportDetails>> candidates;
    
    // Перебираем GPU для поиска подходящего устройства
    for (const VkPhysicalDevice& device: devices) {
        // Смотрим - есть ли у данного устройства поддержка свопа кадров в виде расширения?
        bool swapchainExtentionSupported = checkDeviceRequiredExtensionSupport(device);
        if(swapchainExtentionSupported == false){
            continue;
        }

		// Смотрим - есть ли у данного устройства поддержка нужных слоев
		bool swapchainLayersSupported = checkDeviceRequiredLayerSupport(device);
		if (swapchainLayersSupported == false) {
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
        LOG("No picked GPU physical devices!");
        throw std::runtime_error("No picked GPU physical devices!");
    }
    
    // Получаем наилучший вариант GPU
    if (candidates.begin()->first > 0) {
        _device = std::get<0>(candidates.begin()->second);
        _queuesFamiliesIndexes = std::get<1>(candidates.begin()->second);
        _swapchainSuppportDetails = std::get<2>(candidates.begin()->second);
        
        // Свойства устройства
        vkGetPhysicalDeviceProperties(_device, &_deviceProperties);
        
        // Фичи физического устройства
        vkGetPhysicalDeviceFeatures(_device, &_deviceFeatures);
    } else {
        LOG("Failed to find a suitable GPU!");
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

VulkanPhysicalDevice::~VulkanPhysicalDevice(){
}

VkPhysicalDevice VulkanPhysicalDevice::getDevice() const{
    return _device;
}

const VkPhysicalDeviceFeatures& VulkanPhysicalDevice::getPossibleDeviceFeatures() const{
    return _deviceFeatures;
}

const VkPhysicalDeviceProperties& VulkanPhysicalDevice::getDeviceProperties() const{
    return _deviceProperties;
}

VulkanQueuesFamiliesIndexes VulkanPhysicalDevice::getQueuesFamiliesIndexes() const{
    return _queuesFamiliesIndexes;
}

VulkanSwapChainSupportDetails VulkanPhysicalDevice::getSwapChainSupportDetails() const{
    return _swapchainSuppportDetails;
}

VulkanInstancePtr VulkanPhysicalDevice::getBaseInstance() const{
    return _vulkanInstance;
}

std::vector<const char*> VulkanPhysicalDevice::getBaseExtentions() const{
    return _vulkanExtensions;
}

VulkanSurfacePtr VulkanPhysicalDevice::getBaseSurface() const{
    return _vulkanSurface;
}

// Обновляем информацию о свопчейне после ресайза окна
void VulkanPhysicalDevice::updateSwapchainSupportDetails(){
    _swapchainSuppportDetails = querySwapChainSupport(_device);
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
    std::set<std::string> requiredExtensions(_vulkanExtensions.begin(), _vulkanExtensions.end());
    
    // Пытаемся убрать из списка требуемых расширений возможные
    for (const auto& extension : availableExtensions) {
        LOG("Available extention at physical device: %s\n", extension.extensionName);
        requiredExtensions.erase(extension.extensionName);
    }
    
    // Если пусто, значит поддерживается
    return requiredExtensions.empty();
}

// Смотрим - есть ли у данного устройства поддержка нужных слоев
bool VulkanPhysicalDevice::checkDeviceRequiredLayerSupport(VkPhysicalDevice device) {
	// Получаем количество расширений
	uint32_t layersCount = 0;
	vkEnumerateDeviceLayerProperties(device, &layersCount, nullptr);

	// Получаем расширения
	std::vector<VkLayerProperties> availableLayers(layersCount);
	vkEnumerateDeviceLayerProperties(device, &layersCount, availableLayers.data());

	// Список требуемых слоев
	std::set<std::string> requiredLayers(_vulkanLayers.begin(), _vulkanLayers.end());

	// Пытаемся убрать из списка требуемых расширений возможные
	for (const auto& layer : availableLayers) {
		printf("Available layer at physical device: %s\n", layer.layerName);
		fflush(stdout);
		requiredLayers.erase(layer.layerName);
	}

	// Если пусто, значит поддерживается
	return requiredLayers.empty();
}

// Оценка производительности и пригодности конкретной GPU
int VulkanPhysicalDevice::rateDeviceScore(VkPhysicalDevice device) {
    // Свойства физического устройства
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    
    LOG("Test GPU with name: %s, API version: %d.%d.%d,\n",
           deviceProperties.deviceName,
           VK_VERSION_MAJOR(deviceProperties.apiVersion),
           VK_VERSION_MINOR(deviceProperties.apiVersion),
           VK_VERSION_PATCH(deviceProperties.apiVersion));
    
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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _vulkanSurface->getSurface(), &details.capabilities);
    
    // Запрашиваем поддерживаемые форматы буффера цвета
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vulkanSurface->getSurface(), &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vulkanSurface->getSurface(), &formatCount, details.formats.data());
    }
    
    // Запрашиваем поддерживаемые типы отображения кадра
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vulkanSurface->getSurface(), &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vulkanSurface->getSurface(), &presentModeCount, details.presentModes.data());
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
            result.renderQueuesFamilyIndex = i;
            result.renderQueuesFamilyQueuesCount = queueFamily.queueCount;
            result.renderQueuesTimeStampValidBits = queueFamily.timestampValidBits;
        }
        
        // Провеяем, может является ли данная очередь - очередью отображения
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _vulkanSurface->getSurface(), &presentSupport);
        if ((queueFamily.queueCount > 0) && presentSupport) {
            result.presentQueuesFamilyIndex = i;
            result.presentQueuesFamilyQueuesCount = queueFamily.queueCount;
            result.presentQueuesTimeStampValidBits = queueFamily.timestampValidBits;
        }
        
        // Нашли очереди
        if (result.isComplete()) {
            return result;
        }
        
        i++;
    }
    
    return result;
}

