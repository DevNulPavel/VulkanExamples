#ifndef VULKAN_QUERY_POOL_H
#define VULKAN_QUERY_POOL_H

#include <memory>
#include <set>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResource.h"


class VulkanQueryPool {
public:
    VulkanQueryPool(VulkanLogicalDevicePtr device,
                    VkQueryPipelineStatisticFlags flags,
                    uint32_t flagsCount,
                    uint32_t queriesCount);
    ~VulkanQueryPool();
    VkQueryPool getPool() const;
    void resetPool(const VulkanCommandBufferPtr& buffer);
    void beginPool(const VulkanCommandBufferPtr& buffer, uint32_t index, VkQueryControlFlags flags);
    void endPool(const VulkanCommandBufferPtr& buffer, uint32_t index);
    std::vector<std::map<VkQueryPipelineStatisticFlags, uint64_t>> getPoolResults(); // Получение результатов запросов
    
private:
    VulkanLogicalDevicePtr _device;
    VkQueryPipelineStatisticFlags _flags;
    uint32_t _flagsCount;
    uint32_t _queriesCount;
    std::set<VulkanResourcePtr> _usedResources;
    VkQueryPool _pool;
    
private:
};

typedef std::shared_ptr<VulkanQueryPool> VulkanQueryPoolPtr;

#endif
