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
    
private:
    // Создаем буфферы для глубины
    void createDepthResources();
    // Создание рендер прохода
    void createMainRenderPass();
    // Создаем фреймбуфферы для вьюшек изображений окна
    void createWindowFrameBuffers();
    // Создаем структуру дескрипторов для отрисовки (юниформ буффер, семплер и тд)
    void createDescriptorsSetLayout();
};

typedef std::shared_ptr<VulkanRender> VulkanRenderPtr;

#endif
