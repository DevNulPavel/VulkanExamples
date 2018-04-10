#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

#include <vector>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <VulkanHelpers.h>
#include <VulkanInstance.h>
#include <VulkanSurface.h>
#include <VulkanPhysicalDevice.h>
#include <VulkanLogicalDevice.h>
#include <VulkanQueue.h>
#include <VulkanSemafore.h>
#include <VulkanFence.h>
#include <VulkanSwapchain.h>
#include <VulkanImage.h>
#include <VulkanImageView.h>
#include <VulkanRenderPass.h>
#include <VulkanFrameBuffer.h>
#include <VulkanDescriptorSetLayout.h>
#include <VulkanShaderModule.h>
#include <VulkanPipeline.h>
#include <VulkanCommandPool.h>
#include <VulkanCommandBuffer.h>
#include <VulkanSampler.h>
#include <VulkanBuffer.h>
#include <VulkanDescriptorPool.h>
#include <VulkanDescriptorSet.h>

#include "Vertex2D.h"
#include "Vertex3D.h"
#include "ModelUniformBuffer.h"


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
    void updateRender(float delta);
    // Непосредственно отрисовка кадра
    void drawFrame();
    
private:
    VulkanRender();
    ~VulkanRender();
    
public:
    VulkanInstancePtr vulkanInstance;
    VulkanSurfacePtr vulkanWindowSurface;
    VulkanPhysicalDevicePtr vulkanPhysicalDevice;
    VulkanLogicalDevicePtr vulkanLogicalDevice;
    VulkanQueuePtr vulkanRenderQueue;
    VulkanQueuePtr vulkanPresentQueue;
    VulkanSemaforePtr vulkanImageAvailableSemaphore;
    VulkanSemaforePtr vulkanModelRenderFinishedSemaphore;
    VulkanSemaforePtr vulkanPostRenderFinishedSemaphoreSwapchain;
    VulkanSemaforePtr vulkanPostRenderFinishedSemaphoreModel;
    std::vector<VulkanFencePtr> vulkanPresentFences;
    std::vector<VulkanFencePtr> vulkanRenderFences;
    VulkanSwapchainPtr vulkanSwapchain;
    VulkanCommandPoolPtr vulkanRenderCommandPool;
    VulkanImagePtr postDepthImage;
    std::vector<VulkanFrameBufferPtr> vulkanWindowFrameBuffers;
    std::vector<VulkanCommandBufferPtr> vulkanModelDrawCommandBuffers;
    std::vector<VulkanCommandBufferPtr> vulkanPostDrawCommandBuffers;
    
    VulkanImagePtr postImage;
    VulkanImageViewPtr postImageView;
    VulkanImageViewPtr postDepthImageView;
    VulkanRenderPassPtr vulkanRenderToWindowRenderPass;
    VulkanRenderPassPtr postRenderToRenderPass;
    VulkanFrameBufferPtr postFrameBuffer;
    VulkanDescriptorSetLayoutPtr postDescriptorSetLayout;
    VulkanShaderModulePtr postVertexModule;
    VulkanShaderModulePtr postFragmentModule;
    VulkanPipelinePtr postPipeline;
    VulkanBufferPtr postVertexBuffer;
    VulkanBufferPtr postIndexBuffer;
    VulkanSamplerPtr postTextureSampler;
    VulkanDescriptorPoolPtr postDescriptorPool;
    VulkanDescriptorSetPtr postDescriptorSet;
    std::vector<VulkanCommandBufferPtr> postDrawCommandBuffers;
    
    VulkanDescriptorSetLayoutPtr modelDescriptorSetLayout;
    VulkanShaderModulePtr modelVertexModule;
    VulkanShaderModulePtr modelFragmentModule;
    VulkanPipelinePtr modelPipeline;
    VulkanImagePtr modelTextureImage;
    VulkanImageViewPtr modelTextureImageView;
    VulkanSamplerPtr modelTextureSampler;
    std::vector<Vertex3D> modelVertices;
    std::vector<uint32_t> modelIndices;
    size_t modelTotalVertexesCount;
    size_t modelTotalIndexesCount;
    uint32_t modelImageIndex;
    VulkanBufferPtr modelVertexBuffer;
    VulkanBufferPtr modelIndexBuffer;
    VulkanBufferPtr modelUniformGPUBuffer;
    VulkanDescriptorPoolPtr modelDescriptorPool;
    VulkanDescriptorSetPtr modelDescriptorSet;
    
	float totalTime;
    float rotateAngle;
    
    uint32_t vulkanImageIndex;
    
private:
    void init(GLFWwindow* window);
    
    // Перестраиваем рендеринг при ошибках или ресайзе
    void rebuildRendering();
    
    // Создаем рабочие объекты Vulkan
    void createSharedVulkanObjects(GLFWwindow* window);
    // Создаем буфферы для глубины
    void createPostDepthResources();
    // Создание рендер прохода
    void createRenderToWindowsRenderPass();
    
    // Создаем картинки для отрисовки в текстуру
    void createPostImageAndView();
    // Создание рендер прохода
    void createRenderToPostRenderPass();
    // Создаем фреймбуффер для отрисовки в текстуру
    void createPostFrameBuffer();
    // Создаем структуру дескрипторов для отрисовки (юниформ буффер, семплер и тд)
    void createPostDescriptorsSetLayout();
    // Грузим шейдеры
    void loadPostShaders();
    // Создание пайплайна отрисовки
    void createPostGraphicsPipeline();
    // Создание буфферов вершин
    void createPostBuffers();
    // Создаем пул дескрипторов ресурсов
    void createPostRenderDescriptorPool();
    // Создаем набор дескрипторов ресурсов
    void createPostRenderDescriptorSet();
    
    // Создаем фреймбуфферы для вьюшек изображений окна
    void createWindowFrameBuffers();
    // Создаем структуру дескрипторов для отрисовки (юниформ буффер, семплер и тд)
    void createModelDescriptorsSetLayout();
    // Грузим шейдеры
    void loadModelShaders();
    // Создание пайплайна отрисовки
    void createModelGraphicsPipeline();
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
    
    // Сбрасываем коммандные буфферы
    void resetCommandBuffers();
    
    // Коммандный буффер рендеринга
    VulkanCommandBufferPtr updateModelRenderCommandBuffer(uint32_t frameIndex);
    VulkanCommandBufferPtr updatePostRenderCommandBuffer(uint32_t frameIndex);
};

typedef std::shared_ptr<VulkanRender> VulkanRenderPtr;

#endif
