#include "VulkanLogicalDevice.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>
#include "Helpers.h"
#include "VulkanQueue.h"


VulkanLogicalDevice::VulkanLogicalDevice(VulkanPhysicalDevicePtr physicalDevice,
                                         VulkanQueuesFamiliesIndexes queuesFamiliesIndexes,
                                         float presetQueuePriority,
                                         uint8_t renderQueuesCount,
                                         const std::vector<float>& renderQueuesPriorities,
                                         std::vector<const char*> validationLayers,
                                         std::vector<const char*> extensions,
                                         VkPhysicalDeviceFeatures deviceFeatures):
    _physicalDevice(physicalDevice),
    _queuesFamiliesIndexes(queuesFamiliesIndexes),
    _presetQueuePriority(presetQueuePriority),
    _renderQueuesCount(renderQueuesCount),
    _renderQueuesPriorities(renderQueuesPriorities),
    _validationLayers(validationLayers),
    _extensions(extensions),
    _deviceFeatures(deviceFeatures),
    _device(VK_NULL_HANDLE){
    
    // Отложенное создание в геттерах из-за shared_ptr
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

VkPhysicalDeviceFeatures VulkanLogicalDevice::getBaseFeatures() const{
    return _deviceFeatures;
}

VkDevice VulkanLogicalDevice::getDevice() {
    createLogicalDeviceAndQueue();
    return _device;
}

std::vector<VulkanQueuePtr> VulkanLogicalDevice::getRenderQueues() {
    createLogicalDeviceAndQueue();
    return _renderQueues;
}

std::shared_ptr<VulkanQueue> VulkanLogicalDevice::getPresentQueue() {
    
    return _presentQueue;
}

// Создаем логическое устройство для выбранного физического устройства + очередь отрисовки
void VulkanLogicalDevice::createLogicalDeviceAndQueue() {
    if (_device == VK_NULL_HANDLE) {
        // Только уникальные индексы очередей
        uint32_t vulkanRenderQueueFamilyIndex = _queuesFamiliesIndexes.renderQueuesFamilyIndex;
        uint32_t vulkanPresentQueueFamilyIndex = _queuesFamiliesIndexes.presentQueuesFamilyIndex;
        
        // Определяем количество создаваемых очередей
        if (vulkanRenderQueueFamilyIndex == vulkanPresentQueueFamilyIndex) {
            LOG("One queue family for render and present\n");
            
            uint32_t vulkanRenderQueueFamilyQueuesCount = _queuesFamiliesIndexes.renderQueuesFamilyQueuesCount;

            uint32_t createCount = 0;
            std::vector<uint32_t> queuesIndexes;
            std::vector<float> priorities;
            // Всего одна очередь на все
            if (vulkanRenderQueueFamilyQueuesCount == 1) {
                LOG("Only one queue for all VulkanQueue objects\n");
                
                createCount = 1;
                for (int i = 0; i < (1 + _renderQueuesCount); i++) {
                    queuesIndexes.push_back(0);
                    priorities.push_back(0.5f);
                }
            }
            // На каждый объект очереди - получим свою логическую очередь если нам хватает очередей настоящих
            else if ((vulkanRenderQueueFamilyQueuesCount >= 2) && (vulkanRenderQueueFamilyQueuesCount >= ((uint32_t)_renderQueuesCount + 1))) {
                LOG("Queue for each VulkanQueue object\n");
                
                createCount = (uint32_t)_renderQueuesCount + 1;
                for (int i = 0; i < createCount; i++) {
                    queuesIndexes.push_back(i);
                    if (i == 0) {
                        priorities.push_back(_presetQueuePriority);
                    }else{
                        priorities.push_back(_renderQueuesPriorities[i - 1]);
                    }
                }
            }
            // Если нехватает настоящих очередей под запрошеные нужды
            else if ((vulkanRenderQueueFamilyQueuesCount >= 2) && (vulkanRenderQueueFamilyQueuesCount < ((uint32_t)_renderQueuesCount + 1))) {
                LOG("Less queues than needed for VulkanQueue objects\n");
                
                createCount = (uint32_t)vulkanRenderQueueFamilyQueuesCount;
                for (int i = 0; i < (uint32_t)_renderQueuesCount + 1; i++) {
                    if (i < vulkanRenderQueueFamilyQueuesCount) {
                        queuesIndexes.push_back(i);
                        if (i == 0) {
                            priorities.push_back(_presetQueuePriority);
                        }else{
                            priorities.push_back(_renderQueuesPriorities[i - 1]);
                        }
                    }else{
                        queuesIndexes.push_back(vulkanRenderQueueFamilyQueuesCount-1);
                        priorities[vulkanRenderQueueFamilyQueuesCount-1] = 0.5f;
                    }
                }
            }else{
                LOG("Queues allocating error\n");
                throw std::runtime_error("Queues allocating error");
            }

            VkDeviceQueueCreateInfo queueCreateInfo = {};
            memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = _queuesFamiliesIndexes.renderQueuesFamilyIndex;
            queueCreateInfo.queueCount = createCount;
            queueCreateInfo.pQueuePriorities = priorities.data();
            
            // Конфиг создания девайса
            VkDeviceCreateInfo createInfo = {};
            memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = 1;
            createInfo.pQueueCreateInfos = &queueCreateInfo; // Информация о создаваемых на девайсе очередях
            createInfo.pEnabledFeatures = &_deviceFeatures;          // Информация о фичах устройства
            createInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
            createInfo.ppEnabledExtensionNames = _extensions.data();     // Список требуемых расширений устройства
            createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());    // Слои валидации что и при создании инстанса
            createInfo.ppEnabledLayerNames = _validationLayers.data();
            
            // Пробуем создать логический девайс на конкретном физическом
            VkResult createStatus = vkCreateDevice(_physicalDevice->getDevice(), &createInfo, nullptr, &_device);
            if (createStatus != VK_SUCCESS) {
                LOG("Failed to create logical device!\n");
                throw std::runtime_error("Failed to create logical device!");
            }

            // Очередь отображения
            VkQueue vulkanPresentQueue = VK_NULL_HANDLE;
            vkGetDeviceQueue(_device, vulkanPresentQueueFamilyIndex, queuesIndexes[0], &vulkanPresentQueue);
            _presentQueue = VulkanQueuePtr(new VulkanQueue(shared_from_this(), vulkanRenderQueueFamilyIndex, queuesIndexes[0], vulkanPresentQueue));
            
            // Очереди рендеринга
            for (int i = 1; i < queuesIndexes.size(); i++) {
                VkQueue vulkanGraphicsQueue = VK_NULL_HANDLE;
                vkGetDeviceQueue(_device, vulkanRenderQueueFamilyIndex, queuesIndexes[i], &vulkanGraphicsQueue);
                VulkanQueuePtr renderQueue = VulkanQueuePtr(new VulkanQueue(shared_from_this(), vulkanRenderQueueFamilyIndex, queuesIndexes[i], vulkanGraphicsQueue));
                _renderQueues.push_back(renderQueue);
            }
        }else{
            LOG("Different queue families for render and present\n");
            
            // Present
            VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
            memset(&presentQueueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
            presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            presentQueueCreateInfo.queueFamilyIndex = _queuesFamiliesIndexes.presentQueuesFamilyIndex;
            presentQueueCreateInfo.queueCount = 1;
            presentQueueCreateInfo.pQueuePriorities = &_presetQueuePriority;
            
            // Render
            uint32_t renderQueueFamilyQueuesCount = _queuesFamiliesIndexes.renderQueuesFamilyQueuesCount;
            uint32_t renderQueuesCreateCount = 0;
            std::vector<uint32_t> queuesIndexes;
            std::vector<float> priorities;
            // Всего одна очередь на все
            if (renderQueueFamilyQueuesCount == 1) {
                LOG("Only one queue for all VulkanQueue objects\n");
                renderQueuesCreateCount = 1;
                for (int i = 0; i < _renderQueuesCount; i++) {
                    queuesIndexes.push_back(0);
                    priorities.push_back(0.5f);
                }
            }
            // На каждый объект очереди - получим свою логическую очередь если нам хватает очередей настоящих
            else if ((renderQueueFamilyQueuesCount >= 2) && (renderQueueFamilyQueuesCount >= (uint32_t)_renderQueuesCount)) {
                LOG("Queue for each VulkanQueue object\n");
                renderQueuesCreateCount = (uint32_t)_renderQueuesCount;
                for (int i = 0; i < renderQueuesCreateCount; i++) {
                    queuesIndexes.push_back(i);
                    priorities.push_back(_renderQueuesPriorities[i]);
                }
            }
            // Если нехватает настоящих очередей под запрошеные нужды
            else if ((renderQueueFamilyQueuesCount >= 2) && (renderQueueFamilyQueuesCount < ((uint32_t)_renderQueuesCount))) {
                LOG("Less queues than needed for VulkanQueue objects\n");
                renderQueuesCreateCount = (uint32_t)renderQueueFamilyQueuesCount;
                for (int i = 0; i < (uint32_t)_renderQueuesCount; i++) {
                    if (i < renderQueueFamilyQueuesCount) {
                        queuesIndexes.push_back(i);
                        priorities.push_back(_renderQueuesPriorities[i]);
                    }else{
                        queuesIndexes.push_back(renderQueueFamilyQueuesCount);
                        priorities[renderQueueFamilyQueuesCount] = 0.5f;
                    }
                }
            }else{
                LOG("Queues allocating error\n");
                throw std::runtime_error("Queues allocating error");
            }
            VkDeviceQueueCreateInfo renderQueueCreateInfo = {};
            memset(&renderQueueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
            renderQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            renderQueueCreateInfo.queueFamilyIndex = _queuesFamiliesIndexes.renderQueuesFamilyIndex;
            renderQueueCreateInfo.queueCount = renderQueuesCreateCount;
            renderQueueCreateInfo.pQueuePriorities = priorities.data();
            
            // Конфиг создания девайса
            VkDeviceQueueCreateInfo createQueueInfo[2] = {presentQueueCreateInfo, renderQueueCreateInfo};
            VkDeviceCreateInfo createInfo = {};
            memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = 2;
            createInfo.pQueueCreateInfos = createQueueInfo; // Информация о создаваемых на девайсе очередях
            createInfo.pEnabledFeatures = &_deviceFeatures;          // Информация о фичах устройства
            createInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
            createInfo.ppEnabledExtensionNames = _extensions.data();     // Список требуемых расширений устройства
            createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());    // Слои валидации что и при создании инстанса
            createInfo.ppEnabledLayerNames = _validationLayers.data();
            
            // Пробуем создать логический девайс на конкретном физическом
            VkResult createStatus = vkCreateDevice(_physicalDevice->getDevice(), &createInfo, nullptr, &_device);
            if (createStatus != VK_SUCCESS) {
                LOG("Failed to create logical device!\n");
                throw std::runtime_error("Failed to create logical device!");
            }
            
            // Очередь отображения
            VkQueue vulkanPresentQueue = VK_NULL_HANDLE;
            vkGetDeviceQueue(_device, _queuesFamiliesIndexes.presentQueuesFamilyIndex, 0, &vulkanPresentQueue);
            _presentQueue = VulkanQueuePtr(new VulkanQueue(shared_from_this(), _queuesFamiliesIndexes.presentQueuesFamilyIndex, 0, vulkanPresentQueue));
            
            // Очереди рендеринга
            for (int i = 0; i < queuesIndexes.size(); i++) {
                VkQueue vulkanGraphicsQueue = VK_NULL_HANDLE;
                vkGetDeviceQueue(_device, _queuesFamiliesIndexes.renderQueuesFamilyIndex, queuesIndexes[i], &vulkanGraphicsQueue);
                VulkanQueuePtr renderQueue = VulkanQueuePtr(new VulkanQueue(shared_from_this(), _queuesFamiliesIndexes.renderQueuesFamilyIndex, queuesIndexes[i], vulkanGraphicsQueue));
                _renderQueues.push_back(renderQueue);
            }
        }
    }
}

void VulkanLogicalDevice::wait(){
    vkDeviceWaitIdle(_device);
}
