#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include <memory>
#include <set>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanResource.h"
#include "VulkanLogicalDevice.h"
#include "VulkanCommandPool.h"
#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorSet.h"


struct VulkanRenderPassBeginInfo{
    VulkanRenderPassPtr renderPass;
    VulkanFrameBufferPtr framebuffer;
    VkRect2D renderArea;
    std::vector<VkClearValue> clearValues;
};


class VulkanCommandBuffer {
public:
    VulkanCommandBuffer(VulkanLogicalDevicePtr logicalDevice, VulkanCommandPoolPtr pool);
    ~VulkanCommandBuffer();
    VkCommandBuffer getBuffer() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    VulkanCommandPoolPtr getBasePool() const;
    
    void begin(VkCommandBufferUsageFlags usageFlags);
    void begin(VkCommandBufferUsageFlags usageFlags, const VkCommandBufferInheritanceInfo& inheritance);
    void end();
    void reset(VkCommandBufferResetFlags flags);
    
    void cmdBeginRenderPass(const VulkanRenderPassBeginInfo& beginInfo , VkSubpassContents contentsType);
    void cmdEndRenderPass();
    void cmdSetViewport(const VkRect2D& viewport);
    void cmdSetScissor(const VkRect2D& scissor);
    void cmdBindPipeline(const VulkanPipelinePtr& pipeline);
    void cmdBindVertexBuffer(const VulkanBufferPtr& buffer, VkDeviceSize offset);
    void cmdBindVertexBuffers(const std::vector<VulkanBufferPtr>& buffers, const std::vector<VkDeviceSize>& offsets);
    void cmdBindIndexBuffer(const VulkanBufferPtr& buffer, VkDeviceSize offset, VkIndexType type);
    void cmdBindDescriptorSet(const VkPipelineLayout& pipelineLayout, const VulkanDescriptorSetPtr& set, uint32_t offset = 0);
    void cmdBindDescriptorSets(const VkPipelineLayout& pipelineLayout, const std::vector<VulkanDescriptorSetPtr>& sets);
    void cmdBindDescriptorSets(const VkPipelineLayout& pipelineLayout, const std::vector<VulkanDescriptorSetPtr>& sets, const std::vector<uint32_t>& offsets);
    void cmdPushConstants(const VkPipelineLayout& pipelineLayout, VkShaderStageFlags stage, uint32_t offset, uint32_t size, const void* data);
    void cmdDraw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
    
private:
    VulkanLogicalDevicePtr _logicalDevice;
    VulkanCommandPoolPtr _pool;
    std::set<VulkanResourcePtr> _usedObjects;
    VkCommandBuffer _commandBuffer;
    
private:
};

typedef std::shared_ptr<VulkanCommandBuffer> VulkanCommandBufferPtr;

#endif
