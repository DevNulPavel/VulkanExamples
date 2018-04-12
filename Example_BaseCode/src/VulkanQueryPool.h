#ifndef VULKAN_QUERY_POOL_H
#define VULKAN_QUERY_POOL_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanCommandBuffer.h"


class VulkanQueryPool {
public:
    VulkanQueryPool(VulkanLogicalDevicePtr device,
                    VkQueryPipelineStatisticFlags flags,
                    uint32_t flagsCount);
    ~VulkanQueryPool();
    VkQueryPool getPool() const;
    void resetPool(const VulkanCommandBufferPtr& buffer);
    void beginPool(const VulkanCommandBufferPtr& buffer);
    void endPool(const VulkanCommandBufferPtr& buffer);
    std::map<VkQueryPipelineStatisticFlags, uint64_t> getPoolResults(); // Получение результатов запросов
    
private:
    VulkanLogicalDevicePtr _device;
    VkQueryPipelineStatisticFlags _flags;
    uint32_t _flagsCount;
    VkQueryPool _pool;
    
private:
};

typedef std::shared_ptr<VulkanQueryPool> VulkanQueryPoolPtr;

#endif
