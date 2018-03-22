#ifndef VULKANSWAPCHAIN_H
#define VULKANSWAPCHAIN_H


#include "VulkanDevice.h"


struct VulkanSwapchain {
public:
    VulkanDevice* vulkanDevice;
    VkSwapchainKHR vulkanSwapchain;
    std::vector<VkImage> vulkanSwapChainImages;
    VkFormat vulkanSwapChainImageFormat;
    VkFormat vulkanDepthFormat;
    VkExtent2D vulkanSwapChainExtent;
    std::vector<VkImageView> vulkanSwapChainImageViews;

public:
    VulkanSwapchain(VulkanDevice* device);
    ~VulkanSwapchain();

private:
    // Стадии инициализации
    void createSwapChain();     // Создание логики смены кадров
    void createImageViews();    // Создание вьюшек изображений буффера кадра свопчейна
    VkFormat findDepthFormat(); // Подбираем нужный формат глубины

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    // Выбираем нужный формат кадра
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    // Выбор режима представления кадров из буффера
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    // Выбираем размер кадра-свопа
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    // Создание вью для изображения
    void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView);
    // Подбираем формат текстуры в зависимости от доступных на устройстве
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};


#endif
