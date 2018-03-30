#include "VulkanSwapchain.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>
#include "CommonConstants.h"
#include "VulkanQueue.h"


VulkanSwapchain::VulkanSwapchain(VulkanSurfacePtr surface,
                                 VulkanLogicalDevicePtr device,
                                 VulkanQueuesFamiliesIndexes queuesFamilies,
                                 VulkanSwapChainSupportDetails swapChainSupportDetails,
                                 std::shared_ptr<VulkanSwapchain> oldSwapchain):
    _surface(surface),
    _device(device),
    _queuesFamilies(queuesFamilies),
    _swapChainSupportDetails(swapChainSupportDetails),
    _oldSwapchain(oldSwapchain),
    _swapchain(VK_NULL_HANDLE),
    _swapChainImageFormat(VK_FORMAT_UNDEFINED),
    _swapChainExtent(VkExtent2D{0, 0}){
        
    createSwapChain();
    getSwapchainImages();
    makeSwapchainImageViews();
}

VulkanSwapchain::~VulkanSwapchain(){
    _images.clear();
    _imageViews.clear();
    vkDestroySwapchainKHR(_device->getDevice(), _swapchain, nullptr);
}

VkSwapchainKHR VulkanSwapchain::getSwapchain() const{
    return _swapchain;
}

VkFormat VulkanSwapchain::getSwapChainImageFormat() const{
    return _swapChainImageFormat;
}

VkExtent2D VulkanSwapchain::getSwapChainExtent() const{
    return _swapChainExtent;
}

std::vector<VulkanImagePtr> VulkanSwapchain::getImages() const{
    return _images;
}

std::vector<VulkanImageViewPtr> VulkanSwapchain::getImageViews() const{
    return _imageViews;
}

VulkanSurfacePtr VulkanSwapchain::getBaseSurface() const{
    return _surface;
}

VulkanLogicalDevicePtr VulkanSwapchain::getBaseLogicalDevice() const{
    return _device;
}

VulkanQueuesFamiliesIndexes VulkanSwapchain::getBaseQueuesFamiliesIndexes() const{
    return _queuesFamilies;
}

VulkanSwapChainSupportDetails VulkanSwapchain::getBaseSwapChainSupportDetails() const{
    return _swapChainSupportDetails;
}


// Выбираем нужный формат кадра
VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 0) {
        throw std::runtime_error("No available color formats!");
    }
    
    // Выбираем конкретный стандартный формат, если Vulkan не хочет ничего конкретного
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
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
    
    VkExtent2D actualExtent = {WINDOW_WIDTH, WINDOW_HEIGHT};
    
    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
    
    return actualExtent;
}

// Создание логики смены кадров
void VulkanSwapchain::createSwapChain() {
    // Выбираем подходящие форматы пикселя, режима смены кадров, размеры кадра
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(_swapChainSupportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(_swapChainSupportDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(_swapChainSupportDetails.capabilities);
    
    // Получаем количество изображений в смене кадров, +1 для возможности создания тройной буфферизации
    uint32_t imageCount = _swapChainSupportDetails.capabilities.minImageCount + 1;
    // Значение 0 для maxImageCount означает, что объем памяти не ограничен
    if ((_swapChainSupportDetails.capabilities.maxImageCount > 0) && (imageCount > _swapChainSupportDetails.capabilities.maxImageCount)) {
        imageCount = _swapChainSupportDetails.capabilities.maxImageCount;
    }
    
    // Структура настроек создания Swapchain
    VkSwapchainCreateInfoKHR createInfo = {};
    memset(&createInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface->getSurface();         // Плоскость отрисовки текущего окна
    createInfo.minImageCount = imageCount;      // Количество изображений в буффере
    createInfo.imageFormat = surfaceFormat.format; // Формат отображения
    createInfo.imageColorSpace = surfaceFormat.colorSpace;  // Цветовое пространство
    createInfo.imageExtent = extent;    // Границы окна
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // Картинки используются в качестве буффера цвета
    
    // Если у нас разные очереди для рендеринга и отображения -
    int vulkanRenderQueueFamilyIndex = _queuesFamilies.renderQueuesFamilyIndex;
    int vulkanPresentQueueFamilyIndex = _queuesFamilies.presentQueuesFamilyIndex;
    uint32_t queueFamilyIndices[] = {(uint32_t)vulkanRenderQueueFamilyIndex, (uint32_t)vulkanPresentQueueFamilyIndex};
    if (vulkanRenderQueueFamilyIndex != vulkanPresentQueueFamilyIndex) {
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
    
    createInfo.preTransform = _swapChainSupportDetails.capabilities.currentTransform;   // Предварительный трансформ перед отображением графики, VK_SURFACE_TRANSFORM_*
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;  // Должно ли изображение смешиваться с альфа каналом оконной системы? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    // Пересоздание свопчейна
    VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE;
    if (_oldSwapchain) {
        oldSwapChain = _oldSwapchain->getSwapchain();
    }
    createInfo.oldSwapchain = oldSwapChain;
    
    // Создаем свопчейн
    VkResult createStatus = vkCreateSwapchainKHR(_device->getDevice(), &createInfo, nullptr, &_swapchain);
    if (createStatus != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    // Удаляем старый свопчейн, если был, обазательно удаляется после создания нового
    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device->getDevice(), oldSwapChain, nullptr);
        _oldSwapchain = nullptr;
        oldSwapChain = VK_NULL_HANDLE;
    }
    
    // Сохраняем формат и размеры изображения
    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;
}

// Получаем изображения из свопчейна
void VulkanSwapchain::getSwapchainImages(){
    std::vector<VkImage> vulkanSwapChainImages;
    
    // Получаем изображения для отображения
    uint32_t imagesCount = 0;
    vkGetSwapchainImagesKHR(_device->getDevice(), _swapchain, &imagesCount, nullptr);
    vulkanSwapChainImages.resize(imagesCount);
    vkGetSwapchainImagesKHR(_device->getDevice(), _swapchain, &imagesCount, vulkanSwapChainImages.data());
    
    // создаем обертку
    _images.reserve(vulkanSwapChainImages.size());
    for (const VkImage& image: vulkanSwapChainImages) {
        VulkanImagePtr imagePtr = std::make_shared<VulkanImage>(image, _swapChainImageFormat, _swapChainExtent);
        _images.push_back(imagePtr);
    }
}

// Получаем изображения из свопчейна
void VulkanSwapchain::makeSwapchainImageViews(){
    _imageViews.reserve(_images.size());
    for (const VulkanImagePtr& image: _images) {
        VulkanImageViewPtr imageView = std::make_shared<VulkanImageView>(_device, image, VK_IMAGE_ASPECT_COLOR_BIT);
        _imageViews.push_back(imageView);
    }
}

