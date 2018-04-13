#include "VulkanQueryPool.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <array>
#include "Helpers.h"


VulkanQueryPool::VulkanQueryPool(VulkanLogicalDevicePtr device,
                                 VkQueryPipelineStatisticFlags flags,
                                 uint32_t flagsCount,
                                 uint32_t queriesCount):
    _device(device),
    _flags(flags),
    _flagsCount(flagsCount),
    _queriesCount(queriesCount){
    
    VkQueryPoolCreateInfo queryPoolInfo = {};
    memset(&queryPoolInfo, 0, sizeof(VkQueryPoolCreateInfo));
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;    // This query pool will store pipeline statistics
    queryPoolInfo.queryCount = _flagsCount * _queriesCount;
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
                        _flagsCount*_queriesCount);
}

void VulkanQueryPool::beginPool(const VulkanCommandBufferPtr& buffer, uint32_t index, VkQueryControlFlags flags){
    //_usedResources.insert(buffer);
    
    // Start capture of pipeline statistics
    vkCmdBeginQuery(buffer->getBuffer(), _pool, index, flags);
}

void VulkanQueryPool::endPool(const VulkanCommandBufferPtr& buffer, uint32_t index){
    // End capture of pipeline statistics
    vkCmdEndQuery(buffer->getBuffer(), _pool, index);
    
    //_usedResources.erase(buffer);
}

// Получение результатов запросов
std::vector<std::map<VkQueryPipelineStatisticFlags, uint64_t>> VulkanQueryPool::getPoolResults(){
    std::vector<uint64_t> stats(_flagsCount*_queriesCount);
    vkGetQueryPoolResults(_device->getDevice(),
                          _pool,
                          0,    // First query
                          _queriesCount,    // Count
                          _flagsCount * sizeof(uint64_t),
                          stats.data(),
                          sizeof(uint64_t),
                          VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    
    std::vector<std::map<VkQueryPipelineStatisticFlags, uint64_t>> result;
    
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
    
    result.resize(_queriesCount);
    for (uint32_t i = 0; i < _queriesCount; i++) {
        size_t currentIndex = 0;
        for (VkQueryPipelineStatisticFlags testFlag: testFlags) {
            if ((_flags & testFlag) != 0) {
                result[i][testFlag] = stats[currentIndex];
                currentIndex++;
            }
        }
    }
    return result;
}
