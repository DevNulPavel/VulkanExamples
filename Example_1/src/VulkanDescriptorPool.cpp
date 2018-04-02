#include "VulkanDescriptorPool.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "VulkanHelpers.h"
#include "Helpers.h"


VulkanDescriptorPool::VulkanDescriptorPool(VulkanLogicalDevicePtr logicalDevice, const std::vector<VkDescriptorPoolSize>& poolSize):
    _logicalDevice(logicalDevice),
    _poolSize(poolSize){
    
    // Создаем пул
    VkDescriptorPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(_poolSize.size());
    poolInfo.pPoolSizes = _poolSize.data();
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(_logicalDevice->getDevice(), &poolInfo, nullptr, &_pool) != VK_SUCCESS) {
        LOG("Failed to create descriptor pool!\n");
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

VulkanDescriptorPool::~VulkanDescriptorPool(){
    vkDestroyDescriptorPool(_logicalDevice->getDevice(), _pool, nullptr);
}

VulkanLogicalDevicePtr VulkanDescriptorPool::getBaseDevice() const{
    return _logicalDevice;
}

VkDescriptorPool VulkanDescriptorPool::getPool() const{
    return _pool;
}
