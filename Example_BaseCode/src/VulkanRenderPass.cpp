#include "VulkanRenderPass.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <array>
#include "Helpers.h"


VulkanRenderPassConfig::VulkanRenderPassConfig():
    format(VK_FORMAT_UNDEFINED),
    loadOp(VK_ATTACHMENT_LOAD_OP_DONT_CARE),
    storeOp(VK_ATTACHMENT_STORE_OP_DONT_CARE),
    initLayout(VK_IMAGE_LAYOUT_UNDEFINED),
    refLayout(VK_IMAGE_LAYOUT_UNDEFINED),
    finalLayout(VK_IMAGE_LAYOUT_UNDEFINED){
}

VulkanRenderPass::VulkanRenderPass(VulkanLogicalDevicePtr device, const VkRenderPassCreateInfo& customPassInfo):
    _device(device),
    _isCustom(true){
        
    // Создаем рендер-проход
    if (vkCreateRenderPass(_device->getDevice(), &customPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        LOG("Failed to create render pass!");
        throw std::runtime_error("Failed to create render pass!");
    }
}

VulkanRenderPass::VulkanRenderPass(VulkanLogicalDevicePtr device,
                                   const VulkanRenderPassConfig& imageConfig,
                                   const VulkanRenderPassConfig& depthConfig):
    _device(device),
    _imageConfig(imageConfig),
    _depthConfig(depthConfig),
    _isCustom(false){
        
    // Описание подсоединенного буффера цвета
    VkAttachmentDescription colorAttachment = {};
    memset(&colorAttachment, 0, sizeof(VkAttachmentDescription));
    colorAttachment.format = _imageConfig.format;    // Формат буфферов цвета кадра
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = _imageConfig.loadOp;   // Что делать при начале работы с цветом? VK_ATTACHMENT_LOAD_OP_CLEAR
    colorAttachment.storeOp = _imageConfig.storeOp; // После завершения что делать? VK_ATTACHMENT_STORE_OP_STORE
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Что делать с трафаретом при начале
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Что делать с трафаретом после
    colorAttachment.initialLayout = _imageConfig.initLayout;  // VK_IMAGE_LAYOUT_UNDEFINED
    colorAttachment.finalLayout = _imageConfig.finalLayout;  // Изображение показывается в swap chain VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    
    // Описание присоединенного буффера глубины
    VkAttachmentDescription depthAttachment = {};
    memset(&depthAttachment, 0, sizeof(VkAttachmentDescription));
    depthAttachment.format = _depthConfig.format; //  Формат глубины
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Уровень семплирования
    depthAttachment.loadOp = _depthConfig.loadOp;   // Что делать при загрузке буффера глубины?
    depthAttachment.storeOp = _depthConfig.storeOp; // Что делать после отрисовки с буффером глубины - буффер не рисуется, так что пофик на него
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = _depthConfig.initLayout;    // VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    depthAttachment.finalLayout = _depthConfig.finalLayout;     // VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    
    // Референс присоединенного цвета
    VkAttachmentReference colorAttachmentRef = {};
    memset(&colorAttachmentRef, 0, sizeof(VkAttachmentReference));
    colorAttachmentRef.attachment = 0;  // Аттачмент находится по 0му индексу в массиве attachments ниже
    colorAttachmentRef.layout = _imageConfig.refLayout;   // Используется для цвета VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    
    // Референс присоединенного буффера глубины
    VkAttachmentReference depthAttachmentRef = {};
    memset(&depthAttachmentRef, 0, sizeof(VkAttachmentReference));
    depthAttachmentRef.attachment = 1; // Аттачмент глубины находится по 1му индексу в массиве attachments ниже
    depthAttachmentRef.layout = _depthConfig.refLayout;   // Используется для глубины VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    
    // Подпроход, всего один (наверное можно задействовать для постэффектов)
    VkSubpassDescription subPass = {};
    memset(&subPass, 0, sizeof(VkSubpassDescription));
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // Пайплайн будет использоваться для отрисовки графики
    subPass.colorAttachmentCount = 1;    // Один аттачмент цвета
    subPass.pColorAttachments = &colorAttachmentRef;    // Ref аттачмента цвета
    subPass.pDepthStencilAttachment = &depthAttachmentRef;  // Ref аттачмента глубины
    
    /*// Подпроход, всего один (наверное можно задействовать для постэффектов)
     VkSubpassDescription subPass2 = {};
     memset(&subPass2, 0, sizeof(VkSubpassDescription));
     subPass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // Пайплайн будет использоваться для отрисовки графики
     subPass2.colorAttachmentCount = 1;    // Один аттачмент цвета
     subPass2.pColorAttachments = &colorAttachmentRef;    // Ref аттачмента цвета
     subPass2.pDepthStencilAttachment = &depthAttachmentRef;  // Ref аттачмента глубины*/
    
    // Описание создания рендер-прохода
    std::array<VkSubpassDescription, 1> subpasses = {{subPass}};
    std::array<VkAttachmentDescription, 2> attachments = {{colorAttachment, depthAttachment}};  // Индексы и использование указаны выше
    VkRenderPassCreateInfo renderPassInfo = {};
    memset(&renderPassInfo, 0, sizeof(VkRenderPassCreateInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;
    
    // Создаем рендер-проход
    if (vkCreateRenderPass(_device->getDevice(), &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        LOG("Failed to create render pass!");
        throw std::runtime_error("Failed to create render pass!");
    }
}

VulkanRenderPass::VulkanRenderPass(VulkanLogicalDevicePtr device,
                                   const VulkanRenderPassConfig& imageConfig):
    _device(device),
    _imageConfig(imageConfig),
    _isCustom(false){
    
    // Описание подсоединенного буффера цвета
    VkAttachmentDescription colorAttachment = {};
    memset(&colorAttachment, 0, sizeof(VkAttachmentDescription));
    colorAttachment.format = _imageConfig.format;    // Формат буфферов цвета кадра
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = _imageConfig.loadOp;   // Что делать при начале работы с цветом? VK_ATTACHMENT_LOAD_OP_CLEAR
    colorAttachment.storeOp = _imageConfig.storeOp; // После завершения что делать? VK_ATTACHMENT_STORE_OP_STORE
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Что делать с трафаретом при начале
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Что делать с трафаретом после
    colorAttachment.initialLayout = _imageConfig.initLayout;  // TODO: ???
    colorAttachment.finalLayout = _imageConfig.finalLayout;  // Изображение показывается в swap chain
    
    // Референс присоединенного цвета
    VkAttachmentReference colorAttachmentRef = {};
    memset(&colorAttachmentRef, 0, sizeof(VkAttachmentReference));
    colorAttachmentRef.attachment = 0;  // Аттачмент находится по 0му индексу в массиве attachments ниже
    colorAttachmentRef.layout = _imageConfig.refLayout;   // Используется для цвета VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    
    // Подпроход, всего один (наверное можно задействовать для постэффектов)
    VkSubpassDescription subPass = {};
    memset(&subPass, 0, sizeof(VkSubpassDescription));
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;    // Пайплайн будет использоваться для отрисовки графики
    subPass.colorAttachmentCount = 1;    // Один аттачмент цвета
    subPass.pColorAttachments = &colorAttachmentRef;    // Ref аттачмента цвета
    subPass.pDepthStencilAttachment = nullptr;  // Ref аттачмента глубины
    
    // Описание создания рендер-прохода
    std::array<VkSubpassDescription, 1> subpasses = {{subPass}};
    std::array<VkAttachmentDescription, 1> attachments = {{colorAttachment}};  // Индексы и использование указаны выше
    VkRenderPassCreateInfo renderPassInfo = {};
    memset(&renderPassInfo, 0, sizeof(VkRenderPassCreateInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;
    
    // Создаем рендер-проход
    if (vkCreateRenderPass(_device->getDevice(), &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        LOG("Failed to create render pass!");
        throw std::runtime_error("Failed to create render pass!");
    }
}

VulkanRenderPass::~VulkanRenderPass(){
    vkDestroyRenderPass(_device->getDevice(), _renderPass, nullptr);
}


VulkanRenderPassConfig VulkanRenderPass::getBaseImageConfig() const{
    return _imageConfig;
}

VulkanRenderPassConfig VulkanRenderPass::getBaseDepthConfig() const{
    return _depthConfig;
}

VkRenderPass VulkanRenderPass::getPass() const{
    return _renderPass;
}

bool VulkanRenderPass::isCustom() const{
    return _isCustom;
}
