#include "VulkanDescriptorPool.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "VulkanHelpers.h"
#include "Helpers.h"


VulkanDescriptorPool::VulkanDescriptorPool(VulkanLogicalDevicePtr logicalDevice, const std::vector<VkDescriptorPoolSize>& poolSize, uint32_t maxSets):
    _logicalDevice(logicalDevice),
    _poolSize(poolSize),
    _maxSets(maxSets){
    
    // Создаем пул
    VkDescriptorPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(_poolSize.size());
    poolInfo.pPoolSizes = _poolSize.data();
    poolInfo.maxSets = _maxSets;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    
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

std::vector<VkDescriptorPoolSize> VulkanDescriptorPool::getBasePoolSize() const{
    return _poolSize;
}

uint32_t VulkanDescriptorPool::getBaseMaxSets() const{
    return _maxSets;
}

VkDescriptorPool VulkanDescriptorPool::getPool() const{
    return _pool;
}
