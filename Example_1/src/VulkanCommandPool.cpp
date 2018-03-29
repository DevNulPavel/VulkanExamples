#include "VulkanCommandPool.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>


VulkanCommandPool::VulkanCommandPool(VulkanLogicalDevicePtr logicalDevice, uint32_t queuesFamilyIndex):
    _logicalDevice(logicalDevice),
    _queuesFamilyIndex(queuesFamilyIndex){
    
    // Информация о пуле коммандных буфферов
    VkCommandPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkCommandPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queuesFamilyIndex;   // Пулл будет для семейства очередей рендеринга
    poolInfo.flags = 0; // Optional
    
    if (vkCreateCommandPool(_logicalDevice->getDevice(), &poolInfo, nullptr, &_pool) != VK_SUCCESS) {
        printf("Failed to create command pool!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to create command pool!");
    }
}

VulkanCommandPool::~VulkanCommandPool(){
    vkDestroyCommandPool(_logicalDevice->getDevice(), _pool, nullptr);
}

VkCommandPool VulkanCommandPool::getPool() const{
    return _pool;
}

VulkanLogicalDevicePtr VulkanCommandPool::getBaseDevice() const{
    return _logicalDevice;
}

uint32_t VulkanCommandPool::getBaseQueuesFamilyIndex() const{
    return _queuesFamilyIndex;
}

