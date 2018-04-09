#include "VulkanRender.h"
#include <array>
#include <limits>
#include <numeric>
#include "Helpers.h"
#include "Vertex.h"

// TinyObj
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


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
    modelTotalVertexesCount = 0;
    modelTotalIndexesCount = 0;
    modelImageIndex = 0;
    rotateAngle = 0;
    vulkanImageIndex = 0;
}

void VulkanRender::init(GLFWwindow* window){
    // Создание инстанса Vulkan
    vulkanInstance = std::make_shared<VulkanInstance>();
    
    // Создаем плоскость отрисовки
    vulkanWindowSurface = std::make_shared<VulkanSurface>(window, vulkanInstance);
    
    // Получаем физическое устройство
    std::vector<const char*> vulkanInstanceValidationLayers = vulkanInstance->getValidationLayers();
    std::vector<const char*> vulkanDeviceExtensions;
	vulkanDeviceExtensions.push_back("VK_KHR_swapchain");
    vulkanPhysicalDevice = std::make_shared<VulkanPhysicalDevice>(vulkanInstance, vulkanDeviceExtensions, vulkanInstanceValidationLayers, vulkanWindowSurface);

    // Создаем логическое устройство
    VulkanQueuesFamiliesIndexes vulkanQueuesFamiliesIndexes = vulkanPhysicalDevice->getQueuesFamiliesIndexes(); // Получаем индексы семейств очередей для дальнейшего использования
    VulkanSwapChainSupportDetails vulkanSwapchainSuppportDetails = vulkanPhysicalDevice->getSwapChainSupportDetails();    // Получаем возможности свопчейна
    std::vector<float> renderPriorities = {0.5f};
    VkPhysicalDeviceFeatures logicalDeviceFeatures = {};
    vulkanLogicalDevice = std::make_shared<VulkanLogicalDevice>(vulkanPhysicalDevice,
                                                                vulkanQueuesFamiliesIndexes,
                                                                0.5f,
                                                                1,
                                                                renderPriorities,
                                                                vulkanInstanceValidationLayers,
                                                                vulkanDeviceExtensions,
                                                                logicalDeviceFeatures);
    vulkanRenderQueue = vulkanLogicalDevice->getRenderQueues()[0];      // Получаем очередь рендеринга
    vulkanPresentQueue = vulkanLogicalDevice->getPresentQueue();    // Получаем очередь отрисовки
    
    // Создаем семафоры для отображения и ренедринга
    vulkanImageAvailableSemaphore = std::make_shared<VulkanSemafore>(vulkanLogicalDevice);
    vulkanRenderFinishedSemaphore = std::make_shared<VulkanSemafore>(vulkanLogicalDevice);
    
    // Создаем свопчейн + получаем изображения свопчейна
    vulkanSwapchain = std::make_shared<VulkanSwapchain>(vulkanWindowSurface, vulkanLogicalDevice, vulkanQueuesFamiliesIndexes, vulkanSwapchainSuppportDetails, nullptr);
    
    // Создаем барьеры для защиты от переполнения очереди заданий рендеринга
    vulkanRenderFences.reserve(vulkanSwapchain->getImageViews().size());
    vulkanPresentFences.reserve(vulkanSwapchain->getImageViews().size());
    for (size_t i = 0; i < vulkanSwapchain->getImageViews().size(); i++) {
        VulkanFencePtr renderFence = std::make_shared<VulkanFence>(vulkanLogicalDevice, true);
        vulkanRenderFences.push_back(renderFence);
        VulkanFencePtr presentFence = std::make_shared<VulkanFence>(vulkanLogicalDevice, false);
        vulkanPresentFences.push_back(presentFence);
    }
    
    // Создаем пулл комманд для отрисовки
    vulkanRenderCommandPool = std::make_shared<VulkanCommandPool>(vulkanLogicalDevice, vulkanQueuesFamiliesIndexes.renderQueuesFamilyIndex);
    
    // Создаем текстуры для буффера глубины
    createWindowDepthResources();
    
    // Создаем рендер проход
    createMainRenderPass();

    // Создаем фреймбуфферы для вьюшек изображений окна
    createWindowFrameBuffers();
    
    // Создаем структуру дескрипторов для отрисовки (юниформ буффер, семплер и тд)
    createDescriptorsSetLayout();
    
    // Грузим шейдеры
    loadShaders();
    
    // Создание пайплайна отрисовки
    createGraphicsPipeline();
    
    // Грузим текстуру
    modelTextureImage = createTextureImage(vulkanLogicalDevice, vulkanRenderQueue, vulkanRenderCommandPool, "static_res/textures/chalet.jpg");
    
    // Вью для текстуры
    modelTextureImageView = std::make_shared<VulkanImageView>(vulkanLogicalDevice, modelTextureImage, VK_IMAGE_ASPECT_COLOR_BIT);
    
    // Создаем семплер для текстуры
    modelTextureSampler = std::make_shared<VulkanSampler>(vulkanLogicalDevice,
                                                          VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                          VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                          modelTextureImage->getBaseMipmapsCount());
    
    // Грузим данные для модели
    loadModelSrcData();
    
    // Создание буфферов вершин + индексов
    createModelBuffers();
    
    // Создаем буффер юниформов
    createModelUniformBuffer();
    
    // Создаем пул дескрипторов ресурсов
    createModelDescriptorPool();
    
    // Создаем набор дескрипторов ресурсов
    createModelDescriptorSet();
    
    // Создаем коммандные буфферы отрисовки модели
    createRenderModelCommandBuffers();
}

// Ресайз окна
void VulkanRender::windowResized(GLFWwindow* window, uint32_t width, uint32_t height){
    rebuildRendering();
}

// Перестраиваем рендеринг при ошибках
void VulkanRender::rebuildRendering(){
    // Ждем завершения работы Vulkan
    vulkanRenderQueue->wait();
    vulkanPresentQueue->wait();
    vulkanLogicalDevice->wait();
    
    // Обновляем данные о свопчейне для устройства
    vulkanPhysicalDevice->updateSwapchainSupportDetails();
    
    // Создаем свопчейн + получаем изображения свопчейна
    VulkanQueuesFamiliesIndexes vulkanQueuesFamiliesIndexes = vulkanPhysicalDevice->getQueuesFamiliesIndexes(); // Получаем индексы семейств очередей для дальнейшего использования
    VulkanSwapChainSupportDetails vulkanSwapchainSuppportDetails = vulkanPhysicalDevice->getSwapChainSupportDetails();    // Получаем возможности свопчейна
	VulkanSwapchainPtr newVulkanSwapchain = std::make_shared<VulkanSwapchain>(vulkanWindowSurface, vulkanLogicalDevice, vulkanQueuesFamiliesIndexes, vulkanSwapchainSuppportDetails, vulkanSwapchain);
	vulkanSwapchain = nullptr;
	vulkanSwapchain = newVulkanSwapchain;
    
    // Создаем текстуры для буффера глубины
    createWindowDepthResources();
    
    // Создаем фреймбуфферы для вьюшек изображений окна
    createWindowFrameBuffers();
    
    // Создание пайплайна отрисовки
    createGraphicsPipeline();
    
    // Обновление юниформ буффера
    createModelUniformBuffer();
    
    // Создаем пул дескрипторов ресурсов
    createModelDescriptorPool();
    
    // Создаем набор дескрипторов ресурсов
    createModelDescriptorSet();
    
    // Создаем коммандные буфферы отрисовки модели
    createRenderModelCommandBuffers();
    
    // Обнуляем индекс отрисовки
    vulkanImageIndex = 0;
}

// Создаем буфферы для глубины
void VulkanRender::createWindowDepthResources() {
    // Определяем подходящий формат изображения для глубины
    std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormat vulkanDepthFormat = findSupportedFormat(vulkanPhysicalDevice->getDevice(),
                                                     candidates,
                                                     VK_IMAGE_TILING_OPTIMAL,
                                                     VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    
    // Создаем изображение для глубины
    uint32_t width = vulkanSwapchain->getSwapChainExtent().width;
    uint32_t height = vulkanSwapchain->getSwapChainExtent().height;
    vulkanWindowDepthImage = std::make_shared<VulkanImage>(vulkanLogicalDevice,
                                                           VkExtent2D{width, height},               // Размеры
                                                           vulkanDepthFormat,           // Формат текстуры
                                                           VK_IMAGE_TILING_OPTIMAL,     // Оптимальный тайлинг
                                                           VK_IMAGE_LAYOUT_UNDEFINED,   // Лаяут начальной текстуры (must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED)
                                                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // Использоваться будет в качестве аттачмента глубины
                                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится только на GPU
                                                           1);                          // 1 уровень мипмапов
    
    // Создаем вью для изображения буффера глубины
    vulkanWindowDepthImageView = std::make_shared<VulkanImageView>(vulkanLogicalDevice,
                                                                   vulkanWindowDepthImage,
                                                                   VK_IMAGE_ASPECT_DEPTH_BIT);  // Используем как глубину
    
    // Обновляем лаяут текстуры глубины на правильный
    VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(vulkanLogicalDevice, vulkanRenderCommandPool);
    
    VkImageAspectFlags aspectMask;
    aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent(vulkanWindowDepthImage->getBaseFormat())) {
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    
    // Конвертируем в формат, пригодный для глубины
    transitionImageLayout(commandBuffer,
                          vulkanWindowDepthImage,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          0, VK_REMAINING_MIP_LEVELS,   // Оставшиеся
                          aspectMask,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                          0,
                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    
    endAndQueueWaitSingleTimeCommands(commandBuffer, vulkanRenderQueue);
}

// Создание рендер прохода
void VulkanRender::createMainRenderPass(){
    VulkanRenderPassConfig imageConfig;
    imageConfig.format = vulkanSwapchain->getSwapChainImageFormat();
    imageConfig.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Чистим цвет
    imageConfig.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Сохраняем для отрисовки
    imageConfig.initLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageConfig.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageConfig.refLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VulkanRenderPassConfig depthConfig;
    depthConfig.format = vulkanWindowDepthImage->getBaseFormat();
    depthConfig.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Чистим цвет
    depthConfig.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Не важен результат
    depthConfig.initLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthConfig.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthConfig.refLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    vulkanRenderPass = std::make_shared<VulkanRenderPass>(vulkanLogicalDevice, imageConfig, depthConfig);
}

// Создаем фреймбуфферы для вьюшек изображений окна
void VulkanRender::createWindowFrameBuffers(){
    vulkanWindowFrameBuffers.clear();
    
    std::vector<VulkanImageViewPtr> windowImagesViews = vulkanSwapchain->getImageViews();
    vulkanWindowFrameBuffers.reserve(windowImagesViews.size());
    
    for (const VulkanImageViewPtr& view: windowImagesViews) {
        // Вьюшка текстуры отображения + глубины
        std::vector<VulkanImageViewPtr> views;
        views.push_back(view);
        views.push_back(vulkanWindowDepthImageView);
        
        // Создаем фреймбуффер
        VulkanFrameBufferPtr frameBuffer = std::make_shared<VulkanFrameBuffer>(vulkanLogicalDevice, vulkanRenderPass, views, vulkanSwapchain->getSwapChainExtent());
        vulkanWindowFrameBuffers.push_back(frameBuffer);
    }
}

// Создаем структуру дескрипторов для отрисовки (юниформ буффер, семплер и тд)
void VulkanRender::createDescriptorsSetLayout(){
    VulkanDescriptorSetConfig uniformBuffer;
    uniformBuffer.binding = 0;          // Юниформ буффер биндим на 0 индекс
    uniformBuffer.desriptorsCount = 1;  // 1н дескриптор
    uniformBuffer.desriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Тип - юниформ буффер
    uniformBuffer.descriptorStageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Используется в вершинном шейдере
    
    VulkanDescriptorSetConfig sampler;
    sampler.binding = 1;         // Семплер будет на 1м индексе
    sampler.desriptorsCount = 1; // 1н дескриптор
    sampler.desriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // Тип - семплер
    sampler.descriptorStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Используется в фраггментном шейдере

    std::vector<VulkanDescriptorSetConfig> configs;
    configs.push_back(uniformBuffer);
    configs.push_back(sampler);
    
    vulkanDescriptorSetLayout = std::make_shared<VulkanDescriptorSetLayout>(vulkanLogicalDevice, configs);
}

// Грузим шейдеры
void VulkanRender::loadShaders(){
    // Читаем байт-код шейдеров
    std::vector<unsigned char> vertShaderCode = readFile("res/shaders/shader_vert.spv");
    std::vector<unsigned char> fragShaderCode = readFile("res/shaders/shader_frag.spv");
    
    // Создаем шейдерные модули
    vulkanVertexModule = std::make_shared<VulkanShaderModule>(vulkanLogicalDevice, vertShaderCode);
    vulkanFragmentModule = std::make_shared<VulkanShaderModule>(vulkanLogicalDevice, fragShaderCode);
}

// Создание пайплайна отрисовки
void VulkanRender::createGraphicsPipeline() {
    // Описание вершин, шага по вершинам и описание данных
    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();
    
    // Настраиваем вьюпорт
    VkViewport viewport = {};
    memset(&viewport, 0, sizeof(VkViewport));
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkanSwapchain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(vulkanSwapchain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    // Выставляем сциссор
    VkRect2D scissor = {};
    memset(&scissor, 0, sizeof(VkRect2D));
    scissor.offset = {0, 0};
    scissor.extent = vulkanSwapchain->getSwapChainExtent();
    
    // Настройка глубины
    VulkanPipelineDepthConfig depthConfig;
    depthConfig.depthTestEnabled = VK_TRUE;
    depthConfig.depthWriteEnabled = VK_TRUE;
    depthConfig.depthFunc = VK_COMPARE_OP_LESS;
    
    // Настройки кулинга
    VulkanPipelineCullingConfig cullingConfig;
    cullingConfig.frontFace = VK_FRONT_FACE_CLOCKWISE;
    cullingConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    
    // Блендинг
    VulkanPipelineBlendConfig blendConfig;
    blendConfig.enabled = VK_FALSE;
    blendConfig.blendOp = VK_BLEND_OP_ADD;
    blendConfig.srcFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendConfig.dstFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    
    // Push константы
    std::vector<VkPushConstantRange> pushConstants;
    VkPushConstantRange pushConstantRange = {};
    memset(&pushConstantRange, 0, sizeof(VkPushConstantRange));
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstants.push_back(pushConstantRange);
    
    // Динамически изменяемые параметры
    std::vector<VkDynamicState> dynamicStates;
    dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    
    // Пайплайн
    vulkanPipeline = std::make_shared<VulkanPipeline>(vulkanLogicalDevice,
                                                      vulkanVertexModule, vulkanFragmentModule,
                                                      depthConfig,
                                                      bindingDescription,
                                                      attributeDescriptions,
                                                      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                                      viewport,
                                                      scissor,
                                                      cullingConfig,
                                                      blendConfig,
                                                      vulkanDescriptorSetLayout,
                                                      vulkanRenderPass,
                                                      pushConstants,
                                                      dynamicStates);
}


// Грузим данные для модели
void VulkanRender::loadModelSrcData(){
	LOG("Model loading started\n");
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "static_res/models/chalet.obj")) {
        throw std::runtime_error(err);
    }
    
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            
            vertex.color = {1.0f, 1.0f, 1.0f};
            
            modelVertices.push_back(vertex);
            modelIndices.push_back(modelIndices.size());
        }
    }
    
    modelTotalVertexesCount = modelVertices.size();
    modelTotalIndexesCount = modelIndices.size();
    
    LOG("Model loading complete: %lld vertexes, %lld triangles, %lld indexes\n",
           static_cast<long long int>(modelTotalVertexesCount),
           static_cast<long long int>(modelTotalVertexesCount/3),
           static_cast<long long int>(modelTotalIndexesCount));
}

// Создание буфферов вершин
void VulkanRender::createModelBuffers(){
    // Создаем рабочий буффер
    modelVertexBuffer = createBufferForData(vulkanLogicalDevice, vulkanRenderQueue, vulkanRenderCommandPool, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, (unsigned char*)modelVertices.data(), sizeof(modelVertices[0]) * modelVertices.size());
    
    // Чистим исходные данные
    modelVertices.clear();
    
    // Создаем рабочий буффер
    modelIndexBuffer = createBufferForData(vulkanLogicalDevice, vulkanRenderQueue, vulkanRenderCommandPool, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, (unsigned char*)modelIndices.data(), sizeof(modelIndices[0]) * modelIndices.size());

    // Чистим исходные данные
    modelIndices.clear();
}

// Создаем буффер юниформов
void VulkanRender::createModelUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    // Буффер для юниформов для CPU
    modelUniformStagingBuffer = std::make_shared<VulkanBuffer>(vulkanLogicalDevice,
                                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,    // Хранится в оперативке CPU
                                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Буффер может быть использован как источник данных для копирования
                                                               bufferSize);
    
    // Буффер для юниформов на GPU
    modelUniformGPUBuffer = std::make_shared<VulkanBuffer>(vulkanLogicalDevice,
                                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,   // Хранится только на GPU
                                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // Испольузется как получаетель + юниформ буффер
                                                           bufferSize);
    
    // Обновляем юниформ буффер
    UniformBufferObject ubo = {};
    memset(&ubo, 0, sizeof(UniformBufferObject));
    ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), RenderI->vulkanSwapchain->getSwapChainExtent().width / (float)RenderI->vulkanSwapchain->getSwapChainExtent().height, 0.1f, 10.0f);
    
    // GLM был разработан для OpenGL, где координата Y клип координат перевернута,
    // самым простым путем решения данного вопроса будет изменить знак оси Y в матрице проекции
    //ubo.proj[1][1] *= -1;
    
    // Отгружаем данные
    modelUniformStagingBuffer->uploadDataToBuffer((unsigned char*)&ubo, sizeof(UniformBufferObject));
    
    // Закидываем задачу на копирование буффера
    VulkanCommandBufferPtr commandBuffer = beginSingleTimeCommands(vulkanLogicalDevice, vulkanRenderCommandPool);
    copyBuffer(commandBuffer, modelUniformStagingBuffer, modelUniformGPUBuffer);
    endAndQueueWaitSingleTimeCommands(commandBuffer, vulkanRenderQueue);
}

// Создаем пул дескрипторов ресурсов
void VulkanRender::createModelDescriptorPool() {
    // Структура с типами пулов
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(2);
    // Юниформ буффер
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    // Семплер для текстуры
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;
    
    // Создаем пул
    modelDescriptorPool = std::make_shared<VulkanDescriptorPool>(vulkanLogicalDevice, poolSizes, 1);
}

// Создаем набор дескрипторов ресурсов
void VulkanRender::createModelDescriptorSet() {
    modelDescriptorSet = std::make_shared<VulkanDescriptorSet>(vulkanLogicalDevice, vulkanDescriptorSetLayout, modelDescriptorPool);
    
    VulkanDescriptorSetUpdateConfig vertexBufferSet;
    vertexBufferSet.binding = 0; // Биндится на 0м значении в шейдере
    vertexBufferSet.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Тип - юниформ буффер
    vertexBufferSet.bufferInfo.buffer = modelUniformGPUBuffer->getBuffer();
    vertexBufferSet.bufferInfo.offset = 0;
    vertexBufferSet.bufferInfo.range = sizeof(UniformBufferObject);
    
    VulkanDescriptorSetUpdateConfig samplerSet;
    samplerSet.binding = 1; // Биндится на 1м значении в шейдере
    samplerSet.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerSet.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerSet.imageInfo.imageView = modelTextureImageView->getImageView();
    samplerSet.imageInfo.sampler = modelTextureSampler->getSampler();
    
    std::vector<VulkanDescriptorSetUpdateConfig> configs;
    configs.push_back(vertexBufferSet);
    configs.push_back(samplerSet);
    modelDescriptorSet->updateDescriptorSet(configs);
}

VulkanCommandBufferPtr VulkanRender::makeModelCommandBuffer(uint32_t frameIndex){
    VulkanCommandBufferPtr buffer = std::make_shared<VulkanCommandBuffer>(vulkanLogicalDevice, vulkanRenderCommandPool);
    
    // Настройка наследования
    /*VkCommandBufferInheritanceInfo inheritanceInfo = {};
     memset(&inheritanceInfo, 0, sizeof(VkCommandBufferInheritanceInfo));
     inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
     inheritanceInfo.pNext = nullptr;
     inheritanceInfo.renderPass = vulkanRenderPass->getPass();
     inheritanceInfo.subpass = 0;
     inheritanceInfo.framebuffer = vulkanWindowFrameBuffers[i]->getBuffer();
     inheritanceInfo.occlusionQueryEnable = VK_FALSE;
     inheritanceInfo.queryFlags = 0;
     inheritanceInfo.pipelineStatistics = 0;*/
    
    buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); // Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения. VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    
    // Информация о запуске рендер-прохода
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.5f, 0.5f, 0.5f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    VkRenderPassBeginInfo renderPassInfo = {};
    memset(&renderPassInfo, 0, sizeof(VkRenderPassBeginInfo));
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkanRenderPass->getPass();   // Рендер проход
    renderPassInfo.framebuffer = vulkanWindowFrameBuffers[frameIndex]->getBuffer();    // Фреймбуффер смены кадров
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkanSwapchain->getSwapChainExtent();
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();
    
    // Запуск рендер-прохода
    // VK_SUBPASS_CONTENTS_INLINE: Команды render pass будут включены в первичный буфер команд и вторичные буферы команд не будут задействованы.
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: Команды render pass будут выполняться из вторичных буферов.
    vkCmdBeginRenderPass(buffer->getBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Динамически изменяемый параметр в пайплайне
    VkViewport viewport = {};
    memset(&viewport, 0, sizeof(VkViewport));
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkanSwapchain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(vulkanSwapchain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer->getBuffer(), 0, 1, &viewport);
    
    // Динамически изменяемый параметр в пайплайне
    VkRect2D scissor = {};
    memset(&scissor, 0, sizeof(VkRect2D));
    scissor.offset = {0, 0};
    scissor.extent = vulkanSwapchain->getSwapChainExtent();
    vkCmdSetScissor(buffer->getBuffer(), 0, 1, &scissor);
    
    // Устанавливаем пайплайн у коммандного буффера
    vkCmdBindPipeline(buffer->getBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getPipeline());
    
    // Привязываем вершинный буффер к пайлпайну
    VkBuffer vertexBuffers[] = {modelVertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer->getBuffer(), 0, 1, vertexBuffers, offsets);
    
    // Привязываем индексный буффер к пайплайну
    vkCmdBindIndexBuffer(buffer->getBuffer(), modelIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    // Подключаем дескрипторы ресурсов для юниформ буффера и текстуры
    VkDescriptorSet set = modelDescriptorSet->getSet();
    vkCmdBindDescriptorSets(buffer->getBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getLayout(), 0, 1, &set, 0, nullptr);
    
    // Push константы для динамической отрисовки
    glm::mat4 model = glm::rotate(glm::mat4(), glm::radians(rotateAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    vkCmdPushConstants(buffer->getBuffer(),
                       vulkanPipeline->getLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       sizeof(model),
                       (unsigned char*)&model);
    
    // Вызов отрисовки - 3 вершины, 1 инстанс, начинаем с 0 вершины и 0 инстанса
    // vkCmdDraw(vulkanCommandBuffers[i], QUAD_VERTEXES.size(), 1, 0, 0);
    // Вызов поиндексной отрисовки - индексы вершин, один инстанс
    vkCmdDrawIndexed(buffer->getBuffer(), modelTotalIndexesCount, 1, 0, 0, 0);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    
    /*// Начинаем вводить комманды для следующего подпрохода рендеринга
     vkCmdNextSubpass(vulkanCommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
     
     // Устанавливаем пайплайн у коммандного буффера
     vkCmdBindPipeline(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline);
     
     // Привязываем вершинный буффер к пайлпайну
     vkCmdBindVertexBuffers(vulkanCommandBuffers[i], 0, 1, vertexBuffers, offsets);
     
     // Привязываем индексный буффер к пайплайну
     vkCmdBindIndexBuffer(vulkanCommandBuffers[i], vulkanIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
     
     // Подключаем дескрипторы ресурсов для юниформ буффера
     vkCmdBindDescriptorSets(vulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayout, 0, 1, &vulkanDescriptorSet, 0, nullptr);
     
     // Вызов отрисовки - 3 вершины, 1 инстанс, начинаем с 0 вершины и 0 инстанса
     // vkCmdDraw(vulkanCommandBuffers[i], QUAD_VERTEXES.size(), 1, 0, 0);
     // Вызов поиндексной отрисовки - индексы вершин, один инстанс
     vkCmdDrawIndexed(vulkanCommandBuffers[i], vulkanTotalIndexesCount/2, 1, 0, 0, 0);*/
    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Заканчиваем рендер проход
    vkCmdEndRenderPass(buffer->getBuffer());
    
    /*VkImageMemoryBarrier imageMemoryBarrier = {};
     memset(&imageMemoryBarrier, 0, sizeof(VkImageMemoryBarrier));
     imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
     imageMemoryBarrier.pNext = nullptr;
     imageMemoryBarrier.srcAccessMask = 0;
     imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
     imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
     imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
     imageMemoryBarrier.srcQueueFamilyIndex = 0;
     imageMemoryBarrier.dstQueueFamilyIndex = 0;
     imageMemoryBarrier.image = vulkanSwapChainImages[i];
     imageMemoryBarrier.subresourceRange = {VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
     
     vkCmdPipelineBarrier(vulkanCommandBuffers[i],
     VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
     VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
     0,
     0, nullptr,
     0, nullptr,
     1, &imageMemoryBarrier
     );*/
    
    // Заканчиваем подготовку коммандного буффера
	buffer->end();

    return buffer;
}

// Создаем коммандные буфферы отрисовки модели
void VulkanRender::createRenderModelCommandBuffers() {
    // Ресайзим массив
    modelDrawCommandBuffers.clear();
    modelDrawCommandBuffers.resize(vulkanSwapchain->getImageViews().size());
}

// Обновляем юниформ буффер
void VulkanRender::updateRender(float delta){
    rotateAngle += delta * 30.0f;
}

// Непосредственно отрисовка кадра
void VulkanRender::drawFrame() {
#ifdef __APPLE__
    // TODO: Помогает против подвисания на ресайзах и тд
	vulkanRenderQueue->wait();
	vulkanPresentQueue->wait();
#endif
    
    TIME_BEGIN(DRAW_TIME);
    
    // Запрашиваем изображение для отображения из swapchain, время ожидания делаем максимальным
    TIME_BEGIN(NEXT_IMAGE_TIME);
    uint32_t swapchainImageIndex = 0;    // Индекс картинки свопчейна
    VkResult result = vkAcquireNextImageKHR(vulkanLogicalDevice->getDevice(),
                                            vulkanSwapchain->getSwapchain(),
                                            std::numeric_limits<uint64_t>::max(),
                                            vulkanImageAvailableSemaphore->getSemafore(), // Семафор ожидания доступной картинки
                                            /*vulkanPresentFences[vulkanImageIndex]->getFence()*/ VK_NULL_HANDLE,
                                            &swapchainImageIndex);
    TIME_END_MICROSEC(NEXT_IMAGE_TIME, "Next image index wait time");
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        rebuildRendering();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    // Проверяем, совпадает ли номер картинки и индекс картинки свопчейна
    if (vulkanImageIndex != swapchainImageIndex) {
		LOG("Vulkan image index not equal to swapchain image index (swapchain %d, program %d)!\n", swapchainImageIndex, vulkanImageIndex);
    }

	// Ожидаем доступность закидывания задач на рендеринг
	TIME_BEGIN(WAIT_FENCE);
	vulkanRenderFences[vulkanImageIndex]->waitAndReset();
	TIME_END_MICROSEC(WAIT_FENCE, "Fence render wait time");

    //VkCommandBuffer drawBuffer = modelDrawCommandBuffers[vulkanImageIndex]->getBuffer();
    TIME_BEGIN(MAKE_MODEL_DRAW_BUFFER);
    VulkanCommandBufferPtr buffer = makeModelCommandBuffer(vulkanImageIndex);
    modelDrawCommandBuffers[vulkanImageIndex] = buffer;
    VkCommandBuffer drawBuffer = buffer->getBuffer();
    TIME_END_MICROSEC(MAKE_MODEL_DRAW_BUFFER, "Make model draw buffer wait time");

    // Настраиваем отправление в очередь комманд отрисовки
    // http://vulkanapi.ru/2016/11/14/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-29-%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D0%B8%D0%BD%D0%B3-%D0%B8-%D0%BF%D1%80%D0%B5%D0%B4%D1%81%D1%82%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5-hello-wo/
    VkSemaphore waitSemaphores[] = {vulkanImageAvailableSemaphore->getSemafore()}; // Семафор ожидания картинки для вывода туда графики
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};    // Ждать будем c помощью семафора возможности вывода в буфер цвета
    VkSemaphore signalSemaphores[] = {vulkanRenderFinishedSemaphore->getSemafore()}; // Семафор оповещения о завершении рендеринга
    VkSubmitInfo submitInfo = {};
    memset(&submitInfo, 0, sizeof(VkSubmitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;    // Ожидаем доступное изображение, в которое можно было бы записывать пиксели
    submitInfo.pWaitDstStageMask = waitStages;      // Ждать будем возможности вывода в буфер цвета
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawBuffer; // Указываем коммандный буффер отрисовки
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    // Кидаем в очередь задачу на отрисовку с указанным коммандным буффером
	TIME_BEGIN(SUBMIT_TIME);
    if (vkQueueSubmit(vulkanRenderQueue->getQueue(), 1, &submitInfo, vulkanRenderFences[vulkanImageIndex]->getFence()/*VK_NULL_HANDLE*/) != VK_SUCCESS) {
        LOG("Failed to submit draw command buffer!\n");
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
	TIME_END_MICROSEC(SUBMIT_TIME, "Submit wait time");
    
	// Ждем доступности отображения
	TIME_BEGIN(WAIT_FENCE_PRESENT);
	//vulkanPresentFences[vulkanImageIndex]->waitAndReset();
	TIME_END_MICROSEC(WAIT_FENCE_PRESENT, "Present fence wait time");

    // Настраиваем задачу отображения полученного изображения
    VkSwapchainKHR swapChains[] = {vulkanSwapchain->getSwapchain()};
    VkPresentInfoKHR presentInfo = {};
    memset(&presentInfo, 0, sizeof(VkPresentInfoKHR));
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Ожидаем окончания подготовки кадра с помощью семафора
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &swapchainImageIndex;
    
    // Закидываем в очередь задачу отображения картинки
	TIME_BEGIN(PRESENT_DURATION);
    VkResult presentResult = vkQueuePresentKHR(vulkanPresentQueue->getQueue(), &presentInfo);
	TIME_END_MICROSEC(PRESENT_DURATION, "Present wait time");

	// Можно не получать индекс, а просто делать как в Metal, либо на всякий случай получить индекс на старте
	// TODO: Операции на семафорах - нужно ли вообще это???
	vulkanImageIndex = (vulkanImageIndex + 1) % vulkanSwapchain->getImageViews().size();

    // В случае проблем - пересоздаем свопчейн
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        rebuildRendering();
        return;
    } else if (presentResult != VK_SUCCESS) {
		LOG("failed to present swap chain image!\n");
        throw std::runtime_error("failed to present swap chain image!");
    }

    TIME_END_MICROSEC(DRAW_TIME, "Total Draw method time");
	LOG("\n\n");
}

VulkanRender::~VulkanRender(){    
    // Ждем завершения работы Vulkan
    vulkanRenderQueue->wait();
    vulkanPresentQueue->wait();
    vulkanLogicalDevice->wait();
    
    modelDrawCommandBuffers.clear();
    modelDescriptorSet = nullptr;
    modelDescriptorPool = nullptr;
    modelUniformGPUBuffer = nullptr;
    modelUniformStagingBuffer = nullptr;
    modelVertexBuffer = nullptr;
    modelIndexBuffer = nullptr;
    modelTextureSampler = nullptr;
    modelTextureImage = nullptr;
    modelTextureImageView = nullptr;
    vulkanRenderCommandPool = nullptr;
    vulkanPipeline = nullptr;
    vulkanVertexModule = nullptr;
    vulkanFragmentModule = nullptr;
    vulkanDescriptorSetLayout = nullptr;
    vulkanWindowFrameBuffers.clear();
    vulkanRenderPass = nullptr;
    vulkanWindowDepthImageView = nullptr;
    vulkanWindowDepthImage = nullptr;
    vulkanSwapchain = nullptr;
    vulkanPresentFences.clear();
    vulkanRenderFences.clear();
    vulkanImageAvailableSemaphore = nullptr;
    vulkanRenderFinishedSemaphore = nullptr;
    vulkanRenderQueue = nullptr;
    vulkanPresentQueue = nullptr;
    vulkanLogicalDevice = nullptr;
    vulkanPhysicalDevice = nullptr;
    vulkanWindowSurface = nullptr;
    vulkanInstance = nullptr;
}
