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

public:
    VulkanVisualizer(VulkanDevice* device);
    ~VulkanVisualizer();
    void createFramebuffers(VulkanRenderInfo* renderInfo);      // Создаем фреймбуфферы для вьюшек изображений свопчейна

private:
    // Стадии инициализации
    void createSwapChain();         // Создание логики смены кадров
    void getSwapchainImages();      // Получаем изображения из свопчейна
    void createSwapchainImageViews();    // Создание вьюшек изображений буффера кадра свопчейна
    VkFormat findDepthFormat();     // Подбираем нужный формат глубины
    void createDepthResources();    // Создаем буфферы для глубины

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
    // Подбираем тип памяти под свойства
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    // Создаем изображение
    void createImage(uint32_t width, uint32_t height,
                     VkFormat format, VkImageTiling tiling,
                     VkImageLayout layout, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory);
};


#endif
