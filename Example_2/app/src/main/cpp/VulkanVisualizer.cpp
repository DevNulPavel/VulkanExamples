#include "VulkanVisualizer.h"
#include <limits>
#include <cstring>
#include <stdexcept>
#include "SupportFunctions.h"


VulkanSwapchain::VulkanSwapchain(VulkanDevice* device):
    vulkanDevice(device),
    vulkanSwapchain(VK_NULL_HANDLE),
    vulkanSwapChainImageFormat(VK_FORMAT_UNDEFINED),
    vulkanDepthFormat(VK_FORMAT_UNDEFINED),
    vulkanSwapChainExtent(VkExtent2D{0, 0}){

    createSwapChain();
    createImageViews();
    findDepthFormat();
}

VulkanSwapchain::~VulkanSwapchain(){
    for(const auto& imageView: vulkanSwapChainImageViews){
        vkDestroyImageView(vulkanDevice->vulkanLogicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(vulkanDevice->vulkanLogicalDevice, vulkanSwapchain, nullptr);
}

// Выбираем нужный формат кадра
VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 0) {
        LOGE("No available color formats!");
        throw std::runtime_error("No available color formats!");
    }

    // Выбираем конкретный стандартный формат, если Vulkan не хочет ничего конкретного
    if ((availableFormats.size() == 1) && (availableFormats[0].format == VK_FORMAT_UNDEFINED)) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    // Иначе - перебираем список в поисках лучшего
    for (const auto& availableFormat : availableFormats) {
        if ((availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return availableFormat;
        }
    }

    // Если не нашли нужный - просто выбираем первый формат
    return availableFormats[0];
}

// Выбор режима представления кадров из буффера
VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    // Проверяем, можно ли использовать тройную буфферизацию??
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // Если нет - просто двойная буфферизация
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Выбираем размер кадра-свопа
VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {vulkanDevice->windowWidth, vulkanDevice->windowHeight};

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
}

// Создание логики смены кадров
void VulkanSwapchain::createSwapChain() {
    // Выбираем подходящие форматы пикселя, режима смены кадров, размеры кадра
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(vulkanDevice->vulkanSwapChainSupportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(vulkanDevice->vulkanSwapChainSupportDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(vulkanDevice->vulkanSwapChainSupportDetails.capabilities);

    // TODO: ????
    // Получаем количество изображений в смене кадров, +1 для возможности создания тройной буфферизации
    uint32_t imageCount = vulkanDevice->vulkanSwapChainSupportDetails.capabilities.minImageCount + 1;
    // Значение 0 для maxImageCount означает, что объем памяти не ограничен
    if ((vulkanDevice->vulkanSwapChainSupportDetails.capabilities.maxImageCount > 0) &&
            (imageCount > vulkanDevice->vulkanSwapChainSupportDetails.capabilities.maxImageCount)) {
        imageCount = vulkanDevice->vulkanSwapChainSupportDetails.capabilities.maxImageCount;
    }

    // Структура настроек создания Swapchain
    VkSwapchainCreateInfoKHR createInfo = {};
    memset(&createInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vulkanDevice->vulkanSurface;           // Плоскость отрисовки текущего окна
    createInfo.minImageCount = imageCount;                      // Количество изображений в буффере
    createInfo.imageFormat = surfaceFormat.format;              // Формат отображения
    createInfo.imageColorSpace = surfaceFormat.colorSpace;      // Цветовое пространство
    createInfo.imageExtent = extent;                            // Границы окна
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// Картинки используются в качестве буффера цвета

    // Если у нас разные очереди для рендеринга и отображения -
    uint32_t queueFamilyIndices[] = {(uint32_t)vulkanDevice->vulkanFamiliesQueueIndexes.renderQueueFamilyIndex,
                                     (uint32_t)vulkanDevice->vulkanFamiliesQueueIndexes.presentQueueFamilyIndex};
    if (vulkanDevice->vulkanFamiliesQueueIndexes.renderQueueFamilyIndex != vulkanDevice->vulkanFamiliesQueueIndexes.presentQueueFamilyIndex) {
        // Изображение принадлежит одному семейству в один момент времени и должно быть явно передано другому семейству. Данный вариант обеспечивает наилучшую производительность.
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        // Изображение может быть использовано несколькими семействами без явной передачи.
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = vulkanDevice->vulkanSwapChainSupportDetails.capabilities.currentTransform;   // Предварительный трансформ перед отображением графики, VK_SURFACE_TRANSFORM_*
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // Должно ли изображение смешиваться с альфа каналом оконной системы? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    // Пересоздание свопчейна
    VkSwapchainKHR oldSwapChain = vulkanSwapchain;
    createInfo.oldSwapchain = oldSwapChain;

    // Создаем свопчейн
    VkResult createStatus = vkCreateSwapchainKHR(vulkanDevice->vulkanLogicalDevice, &createInfo, nullptr, &vulkanSwapchain);
    if (createStatus != VK_SUCCESS) {
        LOGE("Failed to create swap chain!");
        throw std::runtime_error("Failed to create swap chain!");
    }

    // Удаляем старый свопчейн, если был, обазательно удаляется после создания нового
    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkanDevice->vulkanLogicalDevice, oldSwapChain, nullptr);
        oldSwapChain = VK_NULL_HANDLE;
    }

    // Получаем изображения для отображения
    uint32_t imagesCount = 0;
    vkGetSwapchainImagesKHR(vulkanDevice->vulkanLogicalDevice, vulkanSwapchain, &imagesCount, nullptr);

    vulkanSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanDevice->vulkanLogicalDevice, vulkanSwapchain, &imagesCount, vulkanSwapChainImages.data());

    // Сохраняем формат и размеры изображения
    vulkanSwapChainImageFormat = surfaceFormat.format;
    vulkanSwapChainExtent = extent;
}

// Создание вью для изображения
void VulkanSwapchain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) {
    // Удаляем старый объект, если есть
    if(imageView != VK_NULL_HANDLE){
        vkDestroyImageView(vulkanDevice->vulkanLogicalDevice, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }

    // Описание вьюшки
    VkImageViewCreateInfo viewInfo = {};
    memset(&viewInfo, 0, sizeof(VkImageViewCreateInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image; // Изображение
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 2D
    viewInfo.format = format;   // Формат вьюшки
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;  // Маска по отдольным компонентам??
    viewInfo.subresourceRange.aspectMask = aspectFlags; // Использование вью текстуры
    viewInfo.subresourceRange.baseMipLevel = 0; // 0 мипмаплевел
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    // Создаем имедж вью
    if (vkCreateImageView(vulkanDevice->vulkanLogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        LOGE("Failed to create texture image view!");
        throw std::runtime_error("Failed to create texture image view!");
    }
}

// Создание вьюшек изображений буффера кадра свопчейна
void VulkanSwapchain::createImageViews() {
    // Удаляем старые, если есть
    if(vulkanSwapChainImageViews.size() > 0){
        for(const auto& imageView: vulkanSwapChainImageViews){
            vkDestroyImageView(vulkanDevice->vulkanLogicalDevice, imageView, nullptr);
        }
        vulkanSwapChainImageViews.clear();
    }

    // Ресайз массива
    vulkanSwapChainImageViews.resize(vulkanSwapChainImages.size());

    for (uint32_t i = 0; i < vulkanSwapChainImages.size(); i++) {
        // Создаем вьюшку с типом использования цвета
        vulkanSwapChainImageViews[i] = VK_NULL_HANDLE;
        createImageView(vulkanSwapChainImages[i], vulkanSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, vulkanSwapChainImageViews[i]);
    }
}

// Подбираем формат текстуры в зависимости от доступных на устройстве
VkFormat VulkanSwapchain::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        // Запрашиваем информацию для формата
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vulkanDevice->vulkanPhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

// Подбираем нужный формат глубины
VkFormat VulkanSwapchain::findDepthFormat() {
    std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    vulkanDepthFormat = findSupportedFormat(candidates,
                                            VK_IMAGE_TILING_OPTIMAL,
                                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return vulkanDepthFormat;
}