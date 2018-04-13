#include "VulkanQueryPool.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <array>
#include "Helpers.h"


VulkanQueryPool::VulkanQueryPool(VulkanLogicalDevicePtr device,
                                 VkQueryPipelineStatisticFlags flags,
                                 uint32_t flagsCount):
    _device(device),
    _flags(flags),
    _flagsCount(flagsCount){
    
    VkQueryPoolCreateInfo queryPoolInfo = {};
    memset(&queryPoolInfo, 0, sizeof(VkQueryPoolCreateInfo));
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;    // This query pool will store pipeline statistics
    queryPoolInfo.queryCount = _flagsCount;
    queryPoolInfo.pipelineStatistics = _flags;
        
    VkResult queryResult = vkCreateQueryPool(_device->getDevice(), &queryPoolInfo, NULL, &_pool);
    if (queryResult != VK_SUCCESS) {
        LOG("Failed to create query pool");
        throw std::runtime_error("Failed to create query pool");
    }
}

VulkanQueryPool::~VulkanQueryPool(){
    vkDestroyQueryPool(_device->getDevice(), _pool, nullptr);
}

VkQueryPool VulkanQueryPool::getPool() const{
    return _pool;
}

void VulkanQueryPool::resetPool(const VulkanCommandBufferPtr& buffer){
    vkCmdResetQueryPool(buffer->getBuffer(),
                        _pool,
                        0,
                        _flagsCount);
}

void VulkanQueryPool::beginPool(const VulkanCommandBufferPtr& buffer){
    //_usedResources.insert(buffer);
    
    // Start capture of pipeline statistics
    vkCmdBeginQuery(buffer->getBuffer(), _pool, 0, VK_QUERY_CONTROL_PRECISE_BIT);
}

void VulkanQueryPool::endPool(const VulkanCommandBufferPtr& buffer){
    // End capture of pipeline statistics
    vkCmdEndQuery(buffer->getBuffer(), _pool, 0);
    
    //_usedResources.erase(buffer);
}

// Получение результатов запросов
std::map<VkQueryPipelineStatisticFlags, uint64_t> VulkanQueryPool::getPoolResults(){
    std::vector<uint64_t> stats(_flagsCount);
    vkGetQueryPoolResults(_device->getDevice(),
                          _pool,
                          0,    // First query
                          1,    // Count
                          _flagsCount * sizeof(uint64_t),
                          stats.data(),
                          sizeof(uint64_t),
                          VK_QUERY_RESULT_64_BIT);
    
    std::map<VkQueryPipelineStatisticFlags, uint64_t> result;
    
    std::array<VkQueryPipelineStatisticFlags, 11> testFlags = {{
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT,
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT,
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT,
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT,
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT,
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT,
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT,
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT,
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT,
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT,
        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT
    }};
    size_t currentIndex = 0;
    for (VkQueryPipelineStatisticFlags testFlag: testFlags) {
        if ((_flags & testFlag) != 0) {
            result[testFlag] = stats[currentIndex];
            currentIndex++;
        }
    }
    return result;
}
