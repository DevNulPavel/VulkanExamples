#ifndef VULKAN_RENDER_H
#define VULKAN_RENDER_H

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"
#include "VulkanSurface.h"
#include "VulkanPhysicalDevice.h"



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
    VulkanQueuesFamiliesIndexes vulkanQueuesFamiliesIndexes;
    VulkanSwapChainSupportDetails vulkanSwapchainSuppportDetails;
};

typedef std::shared_ptr<VulkanRender> VulkanRenderPtr;

#endif
