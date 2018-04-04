#include "VulkanSemafore.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "Helpers.h"


VulkanSemafore::VulkanSemafore(VulkanLogicalDevicePtr device):
    _device(device),
    _semafore(VK_NULL_HANDLE){
        
    VkSemaphoreCreateInfo semaphoreInfo = {};
    memset(&semaphoreInfo, 0, sizeof(VkSemaphoreCreateInfo));
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    // Создаем семафор для отображения и для кодирования графики
    if (vkCreateSemaphore(_device->getDevice(), &semaphoreInfo, nullptr, &_semafore) != VK_SUCCESS) {
        LOG("Failed to create semaphores!\n");
        throw std::runtime_error("Failed to create semaphores!");
    }
}

VulkanSemafore::~VulkanSemafore(){
    vkDestroySemaphore(_device->getDevice(), _semafore, nullptr);
}

VkSemaphore VulkanSemafore::getSemafore() const{
    return _semafore;
}

VulkanLogicalDevicePtr VulkanSemafore::getBaseDevice() const{
    return _device;
}
