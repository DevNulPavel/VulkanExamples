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
    std::vector<const char*> vulkanInstanceValidationLayers;
    std::vector<const char*> vulkanInstanceExtensions;
    VulkanSurfacePtr vulkanWindowSurface;
    VulkanPhysicalDevicePtr vulkanPhysicalDevice;
    VulkanQueuesFamiliesIndexes vulkanQueuesFamiliesIndexes;
    VulkanSwapChainSupportDetails vulkanSwapchainSuppportDetails;
    VulkanLogicalDevicePtr vulkanLogicalDevice;
    VulkanQueuePtr vulkanRenderQueue;
    VulkanQueuePtr vulkanPresentQueue;
};

typedef std::shared_ptr<VulkanRender> VulkanRenderPtr;

#endif
