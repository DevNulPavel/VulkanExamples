#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include <memory>
#include <set>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanResource.h"
#include "VulkanResource.h"
#include "VulkanLogicalDevice.h"
#include "VulkanCommandPool.h"
#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorSet.h"
#include "VulkanQueryPool.h"



struct VulkanCommandBufferInheritanceInfo{
    VulkanRenderPassPtr renderPass;
    uint32_t subpass;
    VulkanFrameBufferPtr framebuffer;
    VkBool32 occlusionQueryEnable;
    VkQueryControlFlags queryFlags;
    VkQueryPipelineStatisticFlags pipelineStatistics;
    
    VulkanCommandBufferInheritanceInfo();
};

struct VulkanRenderPassBeginInfo{
    VulkanRenderPassPtr renderPass;
    VulkanFrameBufferPtr framebuffer;
    VkRect2D renderArea;
    std::vector<VkClearValue> clearValues;
    
    VulkanRenderPassBeginInfo();
};

struct VulkanImageBarrierInfo{
    VulkanImagePtr image;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    uint32_t startMipmapLevel;
    uint32_t levelsCount;
    VkImageAspectFlags aspectFlags;
    VkAccessFlags srcAccessBarrier;
    VkAccessFlags dstAccessBarrier;
    
    VulkanImageBarrierInfo();
};

struct VulkanBufferBarrierInfo{
    VulkanBufferPtr buffer;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkDeviceSize offset;
    VkDeviceSize size;
    
    VulkanBufferBarrierInfo();
};

struct VulkanMemoryBarrierInfo{
    // TODO: Дописать
};


class VulkanCommandBuffer: public VulkanResource {
public:
    VulkanCommandBuffer(VulkanLogicalDevicePtr logicalDevice, VulkanCommandPoolPtr pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    ~VulkanCommandBuffer();
    VkCommandBuffer getBuffer() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanCommandPoolPtr getBasePool() const;
    
    void begin(VkCommandBufferUsageFlags usageFlags);
    void begin(VkCommandBufferUsageFlags usageFlags, const VulkanCommandBufferInheritanceInfo& inheritance);
    void end();
    void reset(VkCommandBufferResetFlags flags);
    
    void cmdBeginRenderPass(const VulkanRenderPassBeginInfo& beginInfo , VkSubpassContents contentsType);
    void cmdEndRenderPass();
    void cmdSetViewport(const VkRect2D& viewport);
    void cmdSetScissor(const VkRect2D& scissor);
    void cmdBindPipeline(const VulkanPipelinePtr& pipeline);
    void cmdBindVertexBuffer(const VulkanBufferPtr& buffer, VkDeviceSize offset = 0);
    void cmdBindVertexBuffers(const std::vector<VulkanBufferPtr>& buffers, const std::vector<VkDeviceSize>& offsets);
    void cmdBindIndexBuffer(const VulkanBufferPtr& buffer, VkIndexType type, VkDeviceSize offset = 0);
    void cmdBindDescriptorSet(const VkPipelineLayout& pipelineLayout, const VulkanDescriptorSetPtr& set);
    void cmdBindDescriptorSet(const VkPipelineLayout& pipelineLayout, const VulkanDescriptorSetPtr& set, uint32_t offset);
    void cmdBindDescriptorSets(const VkPipelineLayout& pipelineLayout, const std::vector<VulkanDescriptorSetPtr>& sets);
    void cmdBindDescriptorSets(const VkPipelineLayout& pipelineLayout, const std::vector<VulkanDescriptorSetPtr>& sets, const std::vector<uint32_t>& offsets);
    void cmdPushConstants(const VkPipelineLayout& pipelineLayout, VkShaderStageFlags stage, const void* data, uint32_t size, uint32_t offset = 0);
    void cmdDraw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
    void cmdCopyImage(const VulkanImagePtr& srcImage, const VulkanImagePtr& dstImage, VkImageAspectFlags aspectMask, uint32_t mipLevel = 0);
    void cmdBlitImage(const VkImageBlit& imageBlit, const VulkanImagePtr& srcImage, const VulkanImagePtr& dstImage);
    void cmdCopyBuffer(const VkBufferCopy& copyRegion, const VulkanBufferPtr& srcBuffer, const VulkanBufferPtr& dstBuffer);
    void cmdCopyAllBuffer(const VulkanBufferPtr& srcBuffer, const VulkanBufferPtr& dstBuffer);
    void cmdPipelineBarrier(VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage,
                            VulkanImageBarrierInfo* imageInfo, uint32_t imageInfoCount,
                            VulkanBufferBarrierInfo* bufferInfo, uint32_t bufferInfoCount,
                            VulkanMemoryBarrierInfo* memoryInfo, uint32_t memoryInfoCount);
    void cmdUpdateBuffer(const VulkanBufferPtr& buffer, unsigned char* data, VkDeviceSize size, VkDeviceSize offset = 0);
    void cmdExecuteCommands(const std::vector<std::shared_ptr<VulkanCommandBuffer>>& buffers);
    void cmdWriteTimeStamp(VkPipelineStageFlagBits stage, const VulkanQueryPoolPtr& pool, uint32_t query);

private:
    VulkanLogicalDevicePtr _logicalDevice;
    VulkanCommandPoolPtr _pool;
    std::set<VulkanResourcePtr> _usedObjects;
    VkCommandBuffer _commandBuffer;
    
private:
};

typedef std::shared_ptr<VulkanCommandBuffer> VulkanCommandBufferPtr;

#endif
