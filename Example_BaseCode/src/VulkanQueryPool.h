#ifndef VULKAN_QUERY_POOL_H
#define VULKAN_QUERY_POOL_H

#include <memory>
#include <set>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanResource.h"
#include "VulkanLogicalDevice.h"


class VulkanCommandBuffer;


struct VulkanQueryPoolPipelineStatistics {
    uint32_t flagsCount;
    VkQueryPipelineStatisticFlagBits flags;

    VulkanQueryPoolPipelineStatistics();
};

struct VulkanQueryPoolOcclusion {
    uint32_t occlusionsCount;

    VulkanQueryPoolOcclusion();
};

struct VulkanQueryPoolTimeStamp {
    uint32_t testCount;

    VulkanQueryPoolTimeStamp();
};


class VulkanQueryPool: public VulkanResource {
public:
    VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolPipelineStatistics& stat);
    VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolOcclusion& occlusion);
    VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolTimeStamp& timeStamp);
    ~VulkanQueryPool();
    VkQueryPool getPool() const;
    void resetPool(const std::shared_ptr<VulkanCommandBuffer>& buffer);
    void beginPool(const std::shared_ptr<VulkanCommandBuffer>& buffer, VkQueryControlFlags flags, uint32_t index = 0);
    void endPool(const std::shared_ptr<VulkanCommandBuffer>& buffer, uint32_t index = 0);
    std::map<VkQueryPipelineStatisticFlags, uint64_t> getPoolStatResults(VkQueryResultFlagBits flags = VK_QUERY_RESULT_WAIT_BIT); // Получение результатов запросов
    std::vector<uint64_t> getPoolOcclusionResults(VkQueryResultFlagBits flags = VK_QUERY_RESULT_WAIT_BIT);
    std::vector<uint64_t> getPoolTimeStampResults(VkQueryResultFlagBits flags = VK_QUERY_RESULT_WAIT_BIT);

private:
    VulkanLogicalDevicePtr _device;
    VkQueryType _type;
    union{
        VulkanQueryPoolPipelineStatistics _pipelineStats;
        VulkanQueryPoolOcclusion _occlusionConfig;
        VulkanQueryPoolTimeStamp _timeStampConfig;
    };
    std::set<VulkanResourcePtr> _usedResources;
    VkQueryPool _pool;
    
private:
};

typedef std::shared_ptr<VulkanQueryPool> VulkanQueryPoolPtr;

#endif
