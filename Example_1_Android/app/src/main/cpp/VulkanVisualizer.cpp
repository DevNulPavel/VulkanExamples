#include "VulkanVisualizer.h"
#include <cstring>
#include <limits>
#include <stdexcept>
#include "VulkanDevice.h"
#include "VulkanRenderInfo.h"
#include "SupportFunctions.h"


VulkanVisualizer::VulkanVisualizer(VulkanDevice* device):
    vulkanDevice(device),
    vulkanSwapchain(VK_NULL_HANDLE),
    vulkanSwapChainImageFormat(VK_FORMAT_UNDEFINED),
    vulkanDepthFormat(VK_FORMAT_UNDEFINED),
    vulkanSwapChainExtent(VkExtent2D{0, 0}),
    vulkanDepthImage(VK_NULL_HANDLE),
    vulkanDepthImageMemory(VK_NULL_HANDLE),
    vulkanDepthImageView(VK_NULL_HANDLE){

    createSwapChain();
    getSwapchainImages();
    createSwapchainImageViews();
    findDepthFormat();
    createDepthResources();
    createSemaphores();
}

VulkanVisualizer::~VulkanVisualizer(){
    for (uint32_t i = 0; i < vulkanImageAvailableSemaphores.size(); i++){
        vkDestroySemaphore(vulkanDevice->vulkanLogicalDevice, vulkanImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(vulkanDevice->vulkanLogicalDevice, vulkanRenderFinishedSemaphores[i], nullptr);
    }

    for (const auto& buffer: vulkanSwapChainFramebuffers) {
        vkDestroyFramebuffer(vulkanDevice->vulkanLogicalDevice, buffer, nullptr);
    }
    vkFreeMemory(vulkanDevice->vulkanLogicalDevice, vulkanDepthImageMemory, nullptr);
    vkDestroyImage(vulkanDevice->vulkanLogicalDevice, vulkanDepthImage, nullptr);
    vkDestroyImageView(vulkanDevice->vulkanLogicalDevice, vulkanDepthImageView, nullptr);
    for(const auto& imageView: vulkanSwapChainImageViews){
        vkDestroyImageView(vulkanDevice->vulkanLogicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(vulkanDevice->vulkanLogicalDevice, vulkanSwapchain, nullptr);
}

// Выбираем нужный формат кадра
VkSurfaceFormatKHR VulkanVisualizer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
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
        }else if ((availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB) &&
                  (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return availableFormat;
        }
    }

    // Если не нашли нужный - просто выбираем первый формат
    return availableFormats[0];
}

// Выбор режима представления кадров из буффера
VkPresentModeKHR VulkanVisualizer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    // Проверяем, можно ли использовать тройную буфферизацию??
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) { // VK_PRESENT_MODE_MAILBOX_KHR
            return availablePresentMode;
        }
    }
    // Проверяем, можно ли использовать тройную буфферизацию??
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { // VK_PRESENT_MODE_MAILBOX_KHR
            return availablePresentMode;
        }
    }

    // Если нет - просто двойная буфферизация
    return availablePresentModes[0];
}

// Выбираем размер кадра-свопа
VkExtent2D VulkanVisualizer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {vulkanDevice->windowWidth, vulkanDevice->windowHeight};

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
}

// Создание логики смены кадров
void VulkanVisualizer::createSwapChain() {
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

    //createInfo.preTransform = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
    //vulkanDevice->vulkanSwapChainSupportDetails.capabilities.currentTransform;   // Предварительный трансформ перед отображением графики, VK_SURFACE_TRANSFORM_*
    createInfo.preTransform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;  // Должно ли изображение смешиваться с альфа каналом оконной системы? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
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

    // Сохраняем формат и размеры изображения
    vulkanSwapChainImageFormat = surfaceFormat.format;
    vulkanSwapChainExtent = extent;
}

// Получаем изображения из свопчейна
void VulkanVisualizer::getSwapchainImages(){
    // Получаем изображения для отображения
    uint32_t imagesCount = 0;
    vkGetSwapchainImagesKHR(vulkanDevice->vulkanLogicalDevice, vulkanSwapchain, &imagesCount, nullptr);

    vulkanSwapChainImages.resize(imagesCount);
    vkGetSwapchainImagesKHR(vulkanDevice->vulkanLogicalDevice, vulkanSwapchain, &imagesCount, vulkanSwapChainImages.data());
}

// Создание вьюшек изображений буффера кадра свопчейна
void VulkanVisualizer::createSwapchainImageViews() {
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
        createImageView(vulkanDevice->vulkanLogicalDevice, vulkanSwapChainImages[i], vulkanSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, vulkanSwapChainImageViews[i]);
    }
}

// Подбираем формат текстуры в зависимости от доступных на устройстве
VkFormat VulkanVisualizer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
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
VkFormat VulkanVisualizer::findDepthFormat() {
    std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    vulkanDepthFormat = findSupportedFormat(candidates,
                                            VK_IMAGE_TILING_OPTIMAL,
                                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return vulkanDepthFormat;
}

// Создаем буфферы для глубины
void VulkanVisualizer::createDepthResources() {
    createImage(vulkanDevice->vulkanLogicalDevice, vulkanDevice->vulkanPhysicalDevice,
                vulkanSwapChainExtent.width, vulkanSwapChainExtent.height,
                vulkanDepthFormat,                                  // Формат текстуры
                VK_IMAGE_TILING_OPTIMAL,                            // Оптимальный тайлинг
                VK_IMAGE_LAYOUT_UNDEFINED,   // Лаяут начальной текстуры (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,        // Использоваться будет в качестве аттачмента глубины
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                // Хранится только на GPU
                vulkanDepthImage, vulkanDepthImageMemory);

    // Создаем вью для изображения буффера глубины
    createImageView(vulkanDevice->vulkanLogicalDevice, vulkanDepthImage, vulkanDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, vulkanDepthImageView);
}

// Создаем фреймбуфферы для вьюшек изображений свопчейна
void VulkanVisualizer::createFramebuffers(VulkanRenderInfo* renderInfo){
    // Уничтожаем старые буфферы свопчейнов
    if (vulkanSwapChainFramebuffers.size() > 0) {
        for (const auto& buffer: vulkanSwapChainFramebuffers) {
            vkDestroyFramebuffer(vulkanDevice->vulkanLogicalDevice, buffer, nullptr);
        }
        vulkanSwapChainFramebuffers.clear();
    }

    // Ресайзим массив с фреймбуфферами свопчейна
    vulkanSwapChainFramebuffers.resize(vulkanSwapChainImageViews.size());

    // Для каждой вьюшки картинки создаем  фреймбуффер
    for (size_t i = 0; i < vulkanSwapChainImageViews.size(); i++) {
        // Список аттачментов
        std::array<VkImageView, 2> attachments = {{vulkanSwapChainImageViews[i], vulkanDepthImageView}};

        // Информация для создания фрейб-буфферов
        VkFramebufferCreateInfo framebufferInfo = {};
        memset(&framebufferInfo, 0, sizeof(VkFramebufferCreateInfo));
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderInfo->vulkanRenderPass;  // TODO: !!!! Совместимый рендер-проход (было vulkanRenderPass)
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());   // Аттачменты
        framebufferInfo.pAttachments = attachments.data();      // Данные аттачментов
        framebufferInfo.width = vulkanSwapChainExtent.width;    // Размеры экрана
        framebufferInfo.height = vulkanSwapChainExtent.height;  // Размеры экрана
        framebufferInfo.layers = 1; // TODO: ???

        VkResult createStatus = vkCreateFramebuffer(vulkanDevice->vulkanLogicalDevice, &framebufferInfo, nullptr, &(vulkanSwapChainFramebuffers[i]));
        if (createStatus != VK_SUCCESS) {
            LOGE("Failed to create framebuffer!");
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

// Создаем семафоры для синхронизаций, чтобы не начинался энкодинг, пока не отобразится один из старых кадров
void VulkanVisualizer::createSemaphores(){
    vulkanImageAvailableSemaphores.resize(vulkanSwapChainImageViews.size());
    vulkanRenderFinishedSemaphores.resize(vulkanSwapChainImageViews.size());

    VkSemaphoreCreateInfo semaphoreInfo = {};
    memset(&semaphoreInfo, 0, sizeof(VkSemaphoreCreateInfo));
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Создаем семафор для отображения и для кодирования графики
    for (uint32_t i = 0; i < vulkanSwapChainImageViews.size(); i++){
        if (vkCreateSemaphore(vulkanDevice->vulkanLogicalDevice, &semaphoreInfo, nullptr, &vulkanImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vulkanDevice->vulkanLogicalDevice, &semaphoreInfo, nullptr, &vulkanRenderFinishedSemaphores[i]) != VK_SUCCESS) {
            LOGE("Failed to create semaphores!");
            throw std::runtime_error("Failed to create semaphores!");
        }
    }
}