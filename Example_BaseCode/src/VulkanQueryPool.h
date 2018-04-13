#ifndef VULKAN_QUERY_POOL_H
#define VULKAN_QUERY_POOL_H

#include <memory>
#include <set>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanResource.h"
#include "VulkanLogicalDevice.h"
#include "VulkanCommandBuffer.h"


struct VulkanQueryPoolPipelineStatistics {
	uint32_t flagsCount;
	VkQueryPipelineStatisticFlagBits flags;

	VulkanQueryPoolPipelineStatistics();
};

struct VulkanQueryPoolOcclusion {
	uint32_t occlusionsCount;

	VulkanQueryPoolOcclusion();
};


class VulkanQueryPool {
public:
    VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolPipelineStatistics& stat);
	VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolOcclusion& occlusion);
    ~VulkanQueryPool();
    VkQueryPool getPool() const;
    void resetPool(const VulkanCommandBufferPtr& buffer);
    void beginPool(const VulkanCommandBufferPtr& buffer, VkQueryControlFlags flags, uint32_t index = 0);
    void endPool(const VulkanCommandBufferPtr& buffer, uint32_t index = 0);
    std::map<VkQueryPipelineStatisticFlags, uint64_t> getPoolStatResults(VkQueryResultFlagBits flags = VK_QUERY_RESULT_WAIT_BIT); // Получение результатов запросов
	std::vector<uint64_t> getPoolOcclusionResults(VkQueryResultFlagBits flags = VK_QUERY_RESULT_WAIT_BIT); // Получение результатов запросов
    
private:
    VulkanLogicalDevicePtr _device;
	VkQueryType _type;
	union{
		VulkanQueryPoolPipelineStatistics _pipelineStats;
		VulkanQueryPoolOcclusion _occlusionConfig;
	};
    std::set<VulkanResourcePtr> _usedResources;
    VkQueryPool _pool;
    
private:
};

typedef std::shared_ptr<VulkanQueryPool> VulkanQueryPoolPtr;

#endif
