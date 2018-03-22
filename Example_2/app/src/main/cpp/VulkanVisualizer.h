#ifndef VULKANSWAPCHAIN_H
#define VULKANSWAPCHAIN_H


#include "VulkanDevice.h"


struct VulkanSwapchain {
public:
    VulkanDevice* vulkanDevice;
    VkSwapchainKHR vulkanSwapchain;
    std::vector<VkImage> vulkanSwapChainImages;
    VkFormat vulkanSwapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkFormat vulkanDepthFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D vulkanSwapChainExtent = {0, 0};

public:
    VulkanSwapchain(VulkanDevice* device);
    ~VulkanSwapchain();

private:
    // Стадии инициализации
    void createSwapChain(); // Создание логики смены кадров

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    // Выбираем нужный формат кадра
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    // Выбор режима представления кадров из буффера
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    // Выбираем размер кадра-свопа
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};


#endif
