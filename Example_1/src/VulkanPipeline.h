#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanLogicalDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRenderPass.h"


struct VulkanPipelineDepthConfig{
    VkBool32 depthTestEnabled;
    VkBool32 depthWriteEnabled;
    VkCompareOp depthFunc;
    
    VulkanPipelineDepthConfig();
};

struct VulkanPipelineCullingConfig{
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
    
    VulkanPipelineCullingConfig();
};

struct VulkanPipelineBlendConfig{
    VkBool32 enabled;
    VkBlendFactor srcFactor;
    VkBlendFactor dstFactor;
    VkBlendOp blendOp;
    
    VulkanPipelineBlendConfig();
};

struct VulkanPipeline {
public:
    VulkanPipeline(VulkanLogicalDevicePtr device,
                   VulkanShaderModulePtr vertexShader, VulkanShaderModulePtr fragmentShader,
                   VulkanPipelineDepthConfig depthConfig,
                   VkVertexInputBindingDescription vertexBindingDescription,
                   std::vector<VkVertexInputAttributeDescription> vertexAttributesDescriptions,
                   VkPrimitiveTopology primitivesTypes,
                   VkViewport viewport,
                   VkRect2D scissor,
                   VulkanPipelineCullingConfig cullingConfig,
                   VulkanPipelineBlendConfig blendConfig,
                   VulkanDescriptorSetLayoutPtr descriptorSetLayout,
                   VulkanRenderPassPtr renderPass,
                   const std::vector<VkPushConstantRange>& pushConstants = std::vector<VkPushConstantRange>(),
                   const std::vector<VkDynamicState>& dynamicStates = std::vector<VkDynamicState>());
    ~VulkanPipeline();
    VkPipelineLayout getLayout() const;
    VkPipeline getPipeline() const;
    VulkanLogicalDevicePtr getBaseDevice() const;
    
private:
    VulkanLogicalDevicePtr _device;
    VulkanShaderModulePtr _vertexShader;
    VulkanShaderModulePtr _fragmentShader;
    VulkanPipelineDepthConfig _depthConfig;
    VkVertexInputBindingDescription _vertexBindingDescription;
    std::vector<VkVertexInputAttributeDescription> _vertexAttributesDescriptions;
    VkPrimitiveTopology _primitivesTypes;
    VkViewport _viewport;
    VkRect2D _scissor;
    VulkanPipelineCullingConfig _cullingConfig;
    VulkanPipelineBlendConfig _blendConfig;
    VulkanDescriptorSetLayoutPtr _descriptorSetLayout;
    VulkanRenderPassPtr _renderPass;
    std::vector<VkPushConstantRange> _pushConstants;
    std::vector<VkDynamicState> _dynamicStates;
    
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    
private:
};

typedef std::shared_ptr<VulkanPipeline> VulkanPipelinePtr;

#endif
