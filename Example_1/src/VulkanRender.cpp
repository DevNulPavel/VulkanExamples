#include "VulkanRender.h"

static VulkanRender* renderInstance = nullptr;

void VulkanRender::initInstance(GLFWwindow* window){
    if (renderInstance == nullptr) {
        renderInstance = new VulkanRender();
        renderInstance->init(window);
    }
}

VulkanRender* VulkanRender::getInstance(){
    return renderInstance;
}

void VulkanRender::destroyRender(){
    if (renderInstance){
        delete renderInstance;
        renderInstance = nullptr;
    }
}

VulkanRender::VulkanRender(){
}

void VulkanRender::init(GLFWwindow* window){
    // Создание инстанса Vulkan
    vulkanInstance = std::make_shared<VulkanInstance>();
    vulkanInstanceValidationLayers = vulkanInstance->getValidationLayers();
    vulkanInstanceExtensions = vulkanInstance->getInstanceExtensions();
    
    // Создаем плоскость отрисовки
    vulkanWindowSurface = std::make_shared<VulkanSurface>(window, vulkanInstance);
    
    // Получаем физическое устройство
    vulkanPhysicalDevice = std::make_shared<VulkanPhysicalDevice>(vulkanInstance, vulkanInstanceExtensions, vulkanWindowSurface);
    vulkanQueuesFamiliesIndexes = vulkanPhysicalDevice->getQueuesFamiliesIndexes(); // Получаем индексы семейств очередей для дальнейшего использования
    vulkanSwapchainSuppportDetails = vulkanPhysicalDevice->getSwapChainSupportDetails();    // Получаем возможности свопчейна

    // Создаем логическое устройство
    vulkanLogicalDevice = std::make_shared<VulkanLogicalDevice>(vulkanPhysicalDevice, vulkanQueuesFamiliesIndexes, vulkanInstanceValidationLayers, vulkanInstanceExtensions);
    vulkanRenderQueue = vulkanLogicalDevice->getRenderQueue();      // Получаем очередь рендеринга
    vulkanPresentQueue = vulkanLogicalDevice->getPresentQueue();    // Получаем очередь отрисовки
    
    // Создаем семафоры для отображения и ренедринга
    vulkanImageAvailableSemaphore = std::make_shared<VulkanSemafore>(vulkanLogicalDevice);
    vulkanRenderFinishedSemaphore = std::make_shared<VulkanSemafore>(vulkanLogicalDevice);
    
    // Создаем свопчейн
    vulkanSwapchain = std::make_shared<VulkanSwapchain>(vulkanWindowSurface, vulkanLogicalDevice, vulkanQueuesFamiliesIndexes, vulkanSwapchainSuppportDetails, nullptr);
}

VulkanRender::~VulkanRender(){
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(vulkanRenderQueue->getQueue());
    vkQueueWaitIdle(vulkanPresentQueue->getQueue());
    vkDeviceWaitIdle(vulkanLogicalDevice->getDevice());
    
    vulkanSwapchain = nullptr;
    vulkanRenderQueue = nullptr;
    vulkanPresentQueue = nullptr;
    vulkanLogicalDevice = nullptr;
    vulkanPhysicalDevice = nullptr;
    vulkanWindowSurface = nullptr;
    vulkanInstance = nullptr;
}
