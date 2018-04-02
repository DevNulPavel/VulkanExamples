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

#include "Vertex3D.h"
#include "Vertex2D.h"
#include "UniformBufferModel.h"
#include "UniformBufferPost.h"


#define RenderI VulkanRender::getInstance()

struct VulkanRender {
public:
    static void initInstance(GLFWwindow* window);
    static VulkanRender* getInstance();
    static void destroyRender();

public:
    // Ресайз окна
    void windowResized(GLFWwindow* window, uint32_t width, uint32_t height);
    // Обновляем юниформ буффер
    void updateUniformBuffer(float delta);
    // Непосредственно отрисовка кадра
    void drawFrame();
    
private:
    VulkanRender();
    ~VulkanRender();
    
public:
    // Vulkan
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
    std::vector<VulkanFrameBufferPtr> vulkanWindowFrameBuffers;
    VulkanCommandPoolPtr vulkanRenderCommandPool;
    VulkanSamplerPtr vulkanTextureSampler;
    
    // PostProcess
    VulkanImagePtr postImage;
    VulkanImageViewPtr postImageView;
    VulkanFrameBufferPtr postFrameBuffers;
    VulkanDescriptorSetLayoutPtr postDescriptorSetLayout;
    VulkanShaderModulePtr postVertexModule;
    VulkanShaderModulePtr postFragmentModule;
    VulkanRenderPassPtr postRenderPass;
    VulkanPipelinePtr postPipeline;
    VulkanBufferPtr postVertexBuffer;
    VulkanBufferPtr postIndexBuffer;
    VulkanBufferPtr postUniformStagingBuffer;
    VulkanBufferPtr postUniformGPUBuffer;
    VulkanDescriptorPoolPtr postDescriptorPool;
    VulkanDescriptorSetPtr postDescriptorSet;
    
    // Model
    VulkanShaderModulePtr modelVertexModule;
    VulkanShaderModulePtr modelFragmentModule;
    VulkanPipelinePtr modelPipeline;
    VulkanRenderPassPtr modelRenderPass;
    VulkanDescriptorSetLayoutPtr modelDescriptorSetLayout;
    VulkanImagePtr modelTextureImage;
    VulkanImageViewPtr modelTextureImageView;
    std::vector<Vertex3D> modelVertices;
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
    
    std::vector<VulkanCommandBufferPtr> drawCommandBuffers;
    
    float totalTime;
    float rotateAngle;
    
    uint32_t vulkanImageIndex;
    
private:
    void init(GLFWwindow* window);
    
    // Создаем общие Vulkan объекты
    void initSharedVulkanObjects(GLFWwindow* window);
    // Перестраиваем рендеринг при ошибках или ресайзе
    void rebuildRendering();
    // Создаем буфферы для глубины
    void createWindowDepthResources();
    // Обновляем лаяут текстуры глубины на правильный
    void updateWindowDepthTextureLayout();
    
    // Создание текстуры, в которую будет происходить отрисовка
    void createPostFrameBufferTexture();
    // Создаем фреймбуфферы для отрисовки в текстуру постобработки
    void createPostFrameBuffer();
    // Создаем структуру дескрипторов для постобработки (юниформ буффер, семплер и тд)
    void createPostDescriptorsSetLayout();
    // Загружаем шейдеры постобработки
    void loadPostShaders();
    // Создание рендер прохода
    void createPostRenderPass();
    // Создание пайплайна отрисовки
    void createPostGraphicsPipeline();
    // Создание буфферов вершин
    void createPostBuffers();
    // Создаем буффер юниформов
    void createPostUniformBuffer();
    // Создаем пул дескрипторов ресурсов
    void createPostDescriptorPool();
    // Создаем набор дескрипторов ресурсов
    void createPostDescriptorSet();
    
    // Создание рендер прохода
    void createModelRenderPass();

    // Создаем структуру дескрипторов для отрисовки (юниформ буффер, семплер и тд)
    void createModelDescriptorsSetLayout();
    // Грузим шейдеры модели
    void loadModelShaders();
    // Создание пайплайна отрисовки
    void createModelGraphicsPipeline();
    
    // Создаем фреймбуфферы для отрисовки на экран
    void createWindowFrameBuffers();
    
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
    void createCommandBuffers();
};

typedef std::shared_ptr<VulkanRender> VulkanRenderPtr;

#endif
