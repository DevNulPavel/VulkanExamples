#include "VulkanQueryPool.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <array>
#include "Helpers.h"

VulkanQueryPoolPipelineStatistics::VulkanQueryPoolPipelineStatistics(): flagsCount(0), flags(VK_QUERY_PIPELINE_STATISTIC_FLAG_BITS_MAX_ENUM) {
}

VulkanQueryPoolOcclusion::VulkanQueryPoolOcclusion(): occlusionsCount(0) {
}

VulkanQueryPoolTimeStamp::VulkanQueryPoolTimeStamp(): testCount(0) {
}


VulkanQueryPool::VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolPipelineStatistics& stat):
    _device(device),
    _type(VK_QUERY_TYPE_PIPELINE_STATISTICS),
    _pipelineStats(stat){
    
    VkQueryPoolCreateInfo queryPoolInfo = {};
    memset(&queryPoolInfo, 0, sizeof(VkQueryPoolCreateInfo));
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = _type;
    queryPoolInfo.queryCount = _pipelineStats.flagsCount;
    queryPoolInfo.pipelineStatistics = _pipelineStats.flags;
        
    VkResult queryResult = vkCreateQueryPool(_device->getDevice(), &queryPoolInfo, NULL, &_pool);
    if (queryResult != VK_SUCCESS) {
        LOG("Failed to create query pool");
        throw std::runtime_error("Failed to create query pool");
    }
}

VulkanQueryPool::VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolOcclusion& occlusion) :
    _device(device),
    _type(VK_QUERY_TYPE_OCCLUSION),
    _occlusionConfig(occlusion) {
    
    VkQueryPoolCreateInfo queryPoolInfo = {};
    memset(&queryPoolInfo, 0, sizeof(VkQueryPoolCreateInfo));
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = _type;
    queryPoolInfo.queryCount = _occlusionConfig.occlusionsCount;
    queryPoolInfo.pipelineStatistics = 0;

    VkResult queryResult = vkCreateQueryPool(_device->getDevice(), &queryPoolInfo, NULL, &_pool);
    if (queryResult != VK_SUCCESS) {
        LOG("Failed to create query pool");
        throw std::runtime_error("Failed to create query pool");
    }
}

VulkanQueryPool::VulkanQueryPool(const VulkanLogicalDevicePtr& device, const VulkanQueryPoolTimeStamp& timeStamp) :
    _device(device),
    _type(VK_QUERY_TYPE_TIMESTAMP),
    _timeStampConfig(timeStamp) {

    VkQueryPoolCreateInfo queryPoolInfo = {};
    memset(&queryPoolInfo, 0, sizeof(VkQueryPoolCreateInfo));
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = _type;
    queryPoolInfo.queryCount = _timeStampConfig.testCount;
    queryPoolInfo.pipelineStatistics = 0;

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
    _usedResources.clear();

    switch (_type) {
    case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
        vkCmdResetQueryPool(buffer->getBuffer(), _pool, 0, _pipelineStats.flagsCount);
    }break;
    case VK_QUERY_TYPE_OCCLUSION: {
        vkCmdResetQueryPool(buffer->getBuffer(), _pool, 0, _occlusionConfig.occlusionsCount);
    }break;
    case VK_QUERY_TYPE_TIMESTAMP: {
        vkCmdResetQueryPool(buffer->getBuffer(), _pool, 0, _timeStampConfig.testCount);
    }break;
    }
}

void VulkanQueryPool::beginPool(const VulkanCommandBufferPtr& buffer, VkQueryControlFlags flags, uint32_t index){
    _usedResources.insert(buffer);

    switch (_type) {
    case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
        vkCmdBeginQuery(buffer->getBuffer(), _pool, 0, flags);
    }break;
    case VK_QUERY_TYPE_OCCLUSION: {
        vkCmdBeginQuery(buffer->getBuffer(), _pool, index, flags);
    }break;
    case VK_QUERY_TYPE_TIMESTAMP: {
        vkCmdBeginQuery(buffer->getBuffer(), _pool, index, flags);
    }break;
    }
}

void VulkanQueryPool::endPool(const VulkanCommandBufferPtr& buffer, uint32_t index){
    switch (_type) {
    case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
        vkCmdEndQuery(buffer->getBuffer(), _pool, 0);
    }break;
    case VK_QUERY_TYPE_OCCLUSION: {
        vkCmdEndQuery(buffer->getBuffer(), _pool, index);
    }break;
    case VK_QUERY_TYPE_TIMESTAMP: {
        vkCmdEndQuery(buffer->getBuffer(), _pool, index);
    }break;
    }
}

// Получение результатов запросов
std::map<VkQueryPipelineStatisticFlags, uint64_t> VulkanQueryPool::getPoolStatResults(VkQueryResultFlagBits flags){
    std::map<VkQueryPipelineStatisticFlags, uint64_t> result;
    if (_type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
        std::vector<uint64_t> stats(_pipelineStats.flagsCount);
        vkGetQueryPoolResults(_device->getDevice(), 
                              _pool, 
                              0,
                              1, 
                              _pipelineStats.flagsCount * sizeof(uint64_t), 
                              stats.data(), 
                              sizeof(uint64_t), 
                              VK_QUERY_RESULT_64_BIT | flags);

        const std::array<VkQueryPipelineStatisticFlags, 11> testFlags = { {
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
                VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT } };

        size_t currentIndex = 0;
        for (VkQueryPipelineStatisticFlags testFlag : testFlags) {
            if ((_pipelineStats.flags & testFlag) != 0) {
                result[testFlag] = stats[currentIndex];
                currentIndex++;
            }
        }
    }
    return result;
}

// Получение результатов запросов
std::vector<uint64_t> VulkanQueryPool::getPoolOcclusionResults(VkQueryResultFlagBits flags) {
    if (_type == VK_QUERY_TYPE_OCCLUSION) {
        std::vector<uint64_t> stats(_occlusionConfig.occlusionsCount);
        vkGetQueryPoolResults(_device->getDevice(), 
                              _pool, 
                              0, 
                              _occlusionConfig.occlusionsCount, 
                              _occlusionConfig.occlusionsCount * sizeof(uint64_t), 
                              stats.data(), 
                              sizeof(uint64_t), 
                              VK_QUERY_RESULT_64_BIT | flags);
        return stats;
    }
    return std::vector<uint64_t>();
}

// Получение результатов запросов
std::vector<uint64_t> VulkanQueryPool::getPoolTimeStampResults(VkQueryResultFlagBits flags) {
    if (_type == VK_QUERY_TYPE_TIMESTAMP) {
        std::vector<uint64_t> stats(_timeStampConfig.testCount);
        vkGetQueryPoolResults(_device->getDevice(),
                              _pool,
                              0,
                              _timeStampConfig.testCount,
                              _timeStampConfig.testCount * sizeof(uint64_t),
                              stats.data(),
                              sizeof(uint64_t),
                              VK_QUERY_RESULT_64_BIT | flags);
        return stats;
    }
    return std::vector<uint64_t>();
}