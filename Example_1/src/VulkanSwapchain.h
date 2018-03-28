#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanSwapChainSupportDetails.h"
#include "VulkanQueuesFamiliesIndexes.h"
#include "VulkanSurface.h"
#include "VulkanLogicalDevice.h"


struct VulkanSwapchain {
public:
    VulkanSwapchain(VulkanSurfacePtr surface,
                    VulkanLogicalDevicePtr device,
                    VulkanQueuesFamiliesIndexes queuesFamilies,
                    VulkanSwapChainSupportDetails swapChainSupportDetails,
                    std::shared_ptr<VulkanSwapchain> oldSwapchain);
    ~VulkanSwapchain();
    VkSwapchainKHR getSwapchain() const;
    VkFormat getSwapChainImageFormat() const;
    VkExtent2D getSwapChainExtent() const;
    
private:
    VulkanSurfacePtr _surface;
    VulkanLogicalDevicePtr _device;
    VulkanQueuesFamiliesIndexes _queuesFamilies;
    VulkanSwapChainSupportDetails _swapChainSupportDetails;
    std::shared_ptr<VulkanSwapchain> _oldSwapchain;
    VkSwapchainKHR _swapchain;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    
private:
    // Создание логики смены кадров
    void createSwapChain();
    // Выбираем нужный формат кадра
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    // Выбор режима представления кадров из буффера
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    // Выбираем размер кадра-свопа
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};

typedef std::shared_ptr<VulkanSwapchain> VulkanSwapchainPtr;

#endif
