#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"
#include "VulkanSurface.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanLogicalDevice.h"
#include "VulkanQueue.h"
#include "VulkanSemafore.h"
#include "VulkanSwapchain.h"



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
};

typedef std::shared_ptr<VulkanRender> VulkanRenderPtr;

#endif
