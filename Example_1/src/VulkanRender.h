#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <vector>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanHelpers.h"
#include "VulkanInstance.h"
#include "VulkanSurface.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanLogicalDevice.h"
#include "VulkanQueue.h"
#include "VulkanSemafore.h"
#include "VulkanSwapchain.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanShaderModule.h"
#include "VulkanPipeline.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSampler.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSet.h"

#include "Vertex.h"
#include "UniformBuffer.h"


#define RenderI VulkanRender::getInstance()

struct VulkanRender {
public:
    static void initInstance(GLFWwindow* window);
    static VulkanRender* getInstance();
    static void destroyRender();

private:
    VulkanRender();
    void init(GLFWwindow* window);
    ~VulkanRender();
    
public:
    VulkanInstancePtr vulkanInstance;
    VulkanSurfacePtr vulkanWindowSurface;
    VulkanPhysicalDevicePtr vulkanPhysicalDevice;
    VulkanLogicalDevicePtr vulkanLogicalDevice;
    VulkanQueuePtr vulkanRenderQueue;
    VulkanQueuePtr vulkanPresentQueue;
    VulkanSemaforePtr vulkanImageAvailableSemaphore;
    VulkanSemaforePtr vulkanRenderFinishedSemaphore;
    VulkanSwapchainPtr vulkanSwapchain;
    VulkanImagePtr vulkanWindowDepthImage;
    VulkanImageViewPtr vulkanWindowDepthImageView;
    VulkanRenderPassPtr vulkanRenderPass;
    std::vector<VulkanFrameBufferPtr> vulkanWindowFrameBuffers;
    VulkanDescriptorSetLayoutPtr vulkanDescriptorSetLayout;
    VulkanShaderModulePtr vulkanVertexModule;
    VulkanShaderModulePtr vulkanFragmentModule;
    VulkanPipelinePtr vulkanPipeline;
    VulkanCommandPoolPtr vulkanRenderCommandPool;
    
    VulkanImagePtr modelTextureImage;
    VulkanImageViewPtr modelTextureImageView;
    VulkanSamplerPtr modelTextureSampler;
    std::vector<Vertex> modelVertices;
    std::vector<uint32_t> modelIndices;
    size_t modelTotalVertexesCount;
    size_t modelTotalIndexesCount;
    uint32_t modelImageIndex;
    VulkanBufferPtr modelVertexBuffer;
    VulkanBufferPtr modelIndexBuffer;
    VulkanBufferPtr modelUniformStagingBuffer;
    VulkanBufferPtr modelUniformGPUBuffer;
    VulkanDescriptorPoolPtr modelDescriptorPool;
    VulkanDescriptorSetPtr modelDescriptorSet;
    std::vector<VulkanCommandBufferPtr> modelDrawCommandBuffers;
    
    float rotateAngle;
    
private:
    // Создаем буфферы для глубины
    void createWindowDepthResources();
    // Создание рендер прохода
    void createMainRenderPass();
    // Создаем фреймбуфферы для вьюшек изображений окна
    void createWindowFrameBuffers();
    // Создаем структуру дескрипторов для отрисовки (юниформ буффер, семплер и тд)
    void createDescriptorsSetLayout();
    // Грузим шейдеры
    void loadShaders();
    // Создание пайплайна отрисовки
    void createGraphicsPipeline();
    // Обновляем лаяут текстуры глубины на правильный
    void updateWindowDepthTextureLayout();
    
    // Грузим данные для модели
    void loadModelSrcData();
    // Создание буфферов вершин
    void createModelBuffers();
    // Создаем буффер юниформов
    void createModelUniformBuffer();
    // Создаем пул дескрипторов ресурсов
    void createModelDescriptorPool();
    // Создаем набор дескрипторов ресурсов
    void createModelDescriptorSet();
    // Создаем коммандные буфферы
    void createRenderModelCommandBuffers();
    
    // Обновляем юниформ буффер
    void updateUniformBuffer(float delta);
};

typedef std::shared_ptr<VulkanRender> VulkanRenderPtr;

#endif
