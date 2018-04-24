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
#include "VulkanResource.h"


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

class VulkanPipeline: public VulkanResource {
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
                   std::vector<VulkanDescriptorSetLayoutPtr> descriptorSetLayouts,
                   VulkanRenderPassPtr renderPass,
                   const std::vector<VkPushConstantRange>& pushConstants = std::vector<VkPushConstantRange>(),
                   const std::vector<VkDynamicState>& dynamicStates = std::vector<VkDynamicState>(),
                   VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
                   bool sampleShading = false,
                   float minSampleShading = 0.0f);
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
    std::vector<VulkanDescriptorSetLayoutPtr> _descriptorSetLayouts;
    VulkanRenderPassPtr _renderPass;
    std::vector<VkPushConstantRange> _pushConstants;
    std::vector<VkDynamicState> _dynamicStates;
    VkSampleCountFlagBits _sampleCount;
    bool _sampleShading;
    float _minSampleShading;
    
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    
private:
};

typedef std::shared_ptr<VulkanPipeline> VulkanPipelinePtr;

#endif
