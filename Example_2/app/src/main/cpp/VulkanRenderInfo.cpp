#include "VulkanRenderInfo.h"
#include <cstring>
#include <stdexcept>
#include "VulkanDevice.h"
#include "VulkanVisualizer.h"
#include "SupportFunctions.h"


VulkanRenderInfo::VulkanRenderInfo(VulkanDevice* device, VulkanVisualizer* visualizer):
    vulkanDevice(device),
    vulkanVisualizer(visualizer),
    vulkanRenderPass(VK_NULL_HANDLE){

    createRenderPass();
}

VulkanRenderInfo::~VulkanRenderInfo(){
    vkDestroyRenderPass(vulkanDevice->vulkanLogicalDevice, vulkanRenderPass, nullptr);
}

// Создание рендер-прохода
void VulkanRenderInfo::createRenderPass() {
    // Уничтожаем старый рендер пасс, если был уже
    if(vulkanRenderPass != VK_NULL_HANDLE){
        vkDestroyRenderPass(vulkanDevice->vulkanLogicalDevice, vulkanRenderPass, nullptr);
    }

    // Описание подсоединенного буффера цвета
    VkAttachmentDescription colorAttachment = {};
    memset(&colorAttachment, 0, sizeof(VkAttachmentDescription));
    colorAttachment.format = vulkanVisualizer->vulkanSwapChainImageFormat;    // Формат буфферов цвета кадра
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Что делать при начале работы с цветом?
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // После завершения что делать?
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Что делать с трафаретом при начале
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Что делать с трафаретом после
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // TODO: ???
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Изображение показывается в swap chain

    // Описание присоединенного буффера глубины
    VkAttachmentDescription depthAttachment = {};
    memset(&depthAttachment, 0, sizeof(VkAttachmentDescription));
    depthAttachment.format = vulkanVisualizer->vulkanDepthFormat; //  Формат
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Уровень семплирования
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Что делать при загрузке буффера глубины?
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Что делать после отрисовки с буффером глубины - буффер не рисуется, так что пофик на него
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Референс присоединенного цвета
    VkAttachmentReference colorAttachmentRef = {};
    memset(&colorAttachmentRef, 0, sizeof(VkAttachmentReference));
    colorAttachmentRef.attachment = 0;  // Аттачмент находится по 0му индексу в массиве attachments ниже
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   // Используется для цвета

    // Референс присоединенного буффера глубины
    VkAttachmentReference depthAttachmentRef = {};
    memset(&depthAttachmentRef, 0, sizeof(VkAttachmentReference));
    depthAttachmentRef.attachment = 1; // Аттачмент глубины находится по 1му индексу в массиве attachments ниже
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;   // Используется для глубины

    // Подпроход, всего один (наверное можно задействовать для постэффектов)
    VkSubpassDescription subPass = {};
    memset(&subPass, 0, sizeof(VkSubpassDescription));
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // Пайплайн будет использоваться для отрисовки графики
    subPass.colorAttachmentCount = 1;    // Один аттачмент цвета
    subPass.pColorAttachments = &colorAttachmentRef;    // Ref аттачмента цвета
    subPass.pDepthStencilAttachment = &depthAttachmentRef;  // Ref аттачмента глубины

    // Описание создания рендер-прохода
    std::array<VkAttachmentDescription, 2> attachments = {{colorAttachment, depthAttachment}};  // Индексы и использование указаны выше
    VkRenderPassCreateInfo renderPassInfo = {};
    memset(&renderPassInfo, 0, sizeof(VkRenderPassCreateInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;
    renderPassInfo.dependencyCount = 0;     // TODO: Зависимости?
    renderPassInfo.pDependencies = nullptr;

    // Создаем ренде-проход
    if (vkCreateRenderPass(vulkanDevice->vulkanLogicalDevice, &renderPassInfo, nullptr, &vulkanRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}