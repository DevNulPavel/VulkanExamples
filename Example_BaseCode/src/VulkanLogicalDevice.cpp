#include "VulkanLogicalDevice.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>
#include "VulkanQueue.h"


VulkanLogicalDevice::VulkanLogicalDevice(VulkanPhysicalDevicePtr physicalDevice,
                                         VulkanQueuesFamiliesIndexes queuesFamiliesIndexes,
                                         std::vector<const char*> validationLayers,
                                         std::vector<const char*> extensions):
    _physicalDevice(physicalDevice),
    _queuesFamiliesIndexes(queuesFamiliesIndexes),
    _validationLayers(validationLayers),
    _extensions(extensions),
    _device(VK_NULL_HANDLE){
        
    // Отложенная инициализация в геттерах
}

VulkanLogicalDevice::~VulkanLogicalDevice(){
    // Wait
    wait();
    
    vkDestroyDevice(_device, nullptr);
}

VulkanPhysicalDevicePtr VulkanLogicalDevice::getBasePhysicalDevice() const{
    return _physicalDevice;
}

VulkanQueuesFamiliesIndexes VulkanLogicalDevice::getBaseQueuesFamiliesIndexes() const{
    return _queuesFamiliesIndexes;
}

std::vector<const char*> VulkanLogicalDevice::getBaseValidationLayers() const{
    return _validationLayers;
}

std::vector<const char*> VulkanLogicalDevice::getBaseExtensions() const{
    return _extensions;
}

VkDevice VulkanLogicalDevice::getDevice(){
    createLogicalDeviceAndQueue();
    return _device;
}

std::shared_ptr<VulkanQueue> VulkanLogicalDevice::getRenderQueue(){
    createLogicalDeviceAndQueue();
    return _renderQueue;
}

std::shared_ptr<VulkanQueue> VulkanLogicalDevice::getPresentQueue(){
    createLogicalDeviceAndQueue();
    return _presentQueue;
}

// Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
void VulkanLogicalDevice::createLogicalDeviceAndQueue() {
    if (_device == VK_NULL_HANDLE) {
        // Только уникальные индексы очередей
        uint32_t vulkanRenderQueueFamilyIndex = _queuesFamiliesIndexes.renderQueuesFamilyIndex;
        uint32_t vulkanRenderQueueFamilyQueuesCount = _queuesFamiliesIndexes.renderQueuesFamilyQueuesCount;
        uint32_t vulkanPresentQueueFamilyIndex = _queuesFamiliesIndexes.presentQueuesFamilyIndex;
        //uint32_t vulkanPresentQueueFamilyQueuesCount = _queuesFamiliesIndexes.presentQueuesFamilyQueuesCount;
        
        std::set<uint32_t> uniqueQueueFamilies = {vulkanRenderQueueFamilyIndex, vulkanPresentQueueFamilyIndex};
        
        // Определяем количество создаваемых очередей
        uint32_t createQueuesCount = 1;
        if (vulkanRenderQueueFamilyIndex == vulkanPresentQueueFamilyIndex) {
            // Если в одном семействе больше одной очереди - берем разные, иначе одну
            if (vulkanRenderQueueFamilyQueuesCount > 1) {
                createQueuesCount = 2;
            }else{
                createQueuesCount = 1;
            }
        }else{
            createQueuesCount = 1;
        }
        
        // Создаем экземпляры настроек создания очереди
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority[2] = {0.7f, 0.5f};
        for (int queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = createQueuesCount;
            queueCreateInfo.pQueuePriorities = queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        // Нужные фичи устройства (ничего не указываем)
        VkPhysicalDeviceFeatures deviceFeatures = _physicalDevice->getDeviceFeatures();
        
        // Конфиг создания девайса
        VkDeviceCreateInfo createInfo = {};
        memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data(); // Информация о создаваемых на девайсе очередях
        createInfo.pEnabledFeatures = &deviceFeatures;          // Информация о фичах устройства
        createInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
        createInfo.ppEnabledExtensionNames = _extensions.data();     // Список требуемых расширений устройства
        createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());    // Слои валидации что и при создании инстанса
        createInfo.ppEnabledLayerNames = _validationLayers.data();
        
        // Пробуем создать логический девайс на конкретном физическом
        VkPhysicalDevice physDevice = _physicalDevice->getDevice();
        VkResult createStatus = vkCreateDevice(physDevice, &createInfo, nullptr, &_device);
        if (createStatus != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }
        
        // Если в одном семействе больше одной очереди - берем разные, иначе одну из разных семейств или одного семейства
        uint32_t queuesIndexes[2] = {0 , 0};
        if (createQueuesCount >= 2) {
            queuesIndexes[0] = 0;
            queuesIndexes[1] = 1;
        }else{
            queuesIndexes[0] = 0;
            queuesIndexes[1] = 0;
        }
        
        // Очередь рендеринга
        VkQueue vulkanGraphicsQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(_device, vulkanRenderQueueFamilyIndex, queuesIndexes[0], &vulkanGraphicsQueue);
        _renderQueue = VulkanQueuePtr(new VulkanQueue(shared_from_this(), vulkanRenderQueueFamilyIndex, queuesIndexes[0], vulkanGraphicsQueue));
        // Очередь отображения
        VkQueue vulkanPresentQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(_device, vulkanPresentQueueFamilyIndex, queuesIndexes[1], &vulkanPresentQueue);
        _presentQueue = VulkanQueuePtr(new VulkanQueue(shared_from_this(), vulkanRenderQueueFamilyIndex, queuesIndexes[1], vulkanPresentQueue));
    }
}

void VulkanLogicalDevice::wait(){
    vkDeviceWaitIdle(_device);
}
