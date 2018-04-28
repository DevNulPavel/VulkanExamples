#ifndef VULKANSWAPCHAIN_H
#define VULKANSWAPCHAIN_H

#include <vector>
#include <vulkan_wrapper.h>

struct VulkanDevice;
struct VulkanRenderInfo;


struct VulkanVisualizer {
public:
    VulkanDevice* vulkanDevice;
    VkSwapchainKHR vulkanSwapchain;
    std::vector<VkImage> vulkanSwapChainImages;
    VkFormat vulkanSwapChainImageFormat;
    VkFormat vulkanDepthFormat;
    VkExtent2D vulkanSwapChainExtent;
    std::vector<VkImageView> vulkanSwapChainImageViews;
    VkImage vulkanDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory vulkanDepthImageMemory;
    VkImageView vulkanDepthImageView;
    std::vector<VkFramebuffer> vulkanSwapChainFramebuffers;
    std::vector<VkSemaphore> vulkanImageAvailableSemaphores;
    std::vector<VkSemaphore> vulkanRenderFinishedSemaphores;

public:
    VulkanVisualizer(VulkanDevice* device);
    ~VulkanVisualizer();
    void createFramebuffers(VulkanRenderInfo* renderInfo);      // Создаем фреймбуфферы для вьюшек изображений свопчейна

private:
    // Стадии инициализации
    void createSwapChain();         // Создание логики смены кадров
    void getSwapchainImages();      // Получаем изображения из свопчейна
    void createSwapchainImageViews(); // Создание вьюшек изображений буффера кадра свопчейна
    VkFormat findDepthFormat();     // Подбираем нужный формат глубины
    void createDepthResources();    // Создаем буфферы для глубины
    void createSemaphores();        // Создаем семафоры для синхронизаций, чтобы не начинался энкодинг, пока не отобразится один из старых кадров

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    // Выбираем нужный формат кадра
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    // Выбор режима представления кадров из буффера
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    // Выбираем размер кадра-свопа
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    // Подбираем формат текстуры в зависимости от доступных на устройстве
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};


#endif
