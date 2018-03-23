#include "VulkanRenderInfo.h"
#include <cstring>
#include <stdexcept>
#include "VulkanDevice.h"
#include "VulkanVisualizer.h"
#include "Vertex.h"
#include "SupportFunctions.h"


VulkanRenderInfo::VulkanRenderInfo(VulkanDevice* device, VulkanVisualizer* visualizer, AAssetManager* assetManager):
    vulkanDevice(device),
    vulkanVisualizer(visualizer),
    androidAssetManager(assetManager),
    vulkanRenderPass(VK_NULL_HANDLE),
    vulkanDescriptorSetLayout(VK_NULL_HANDLE),
    vulkanVertexShader(VK_NULL_HANDLE),
    vulkanFragmentShader(VK_NULL_HANDLE),
    vulkanPipelineLayout(VK_NULL_HANDLE),
    vulkanPipeline(VK_NULL_HANDLE),
    vulkanCommandPool(VK_NULL_HANDLE){

    createRenderPass();
    createUniformDescriptorSetLayout();
    createGraphicsPipeline();
    createRenderCommandPool();
    updateDepthTextureLayout();

}

VulkanRenderInfo::~VulkanRenderInfo(){
    vkDestroyCommandPool(vulkanDevice->vulkanLogicalDevice, vulkanCommandPool, nullptr);
    vkDestroyPipeline(vulkanDevice->vulkanLogicalDevice, vulkanPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanDevice->vulkanLogicalDevice, vulkanPipelineLayout, nullptr);
    vkDestroyShaderModule(vulkanDevice->vulkanLogicalDevice, vulkanVertexShader, nullptr);
    vkDestroyShaderModule(vulkanDevice->vulkanLogicalDevice, vulkanFragmentShader, nullptr);
    vkDestroyDescriptorSetLayout(vulkanDevice->vulkanLogicalDevice, vulkanDescriptorSetLayout, nullptr);
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
        LOGE("Failed to create render pass!");
        throw std::runtime_error("Failed to create render pass!");
    }
}

// Создаем дескриптор для буффера юниформов
void VulkanRenderInfo::createUniformDescriptorSetLayout() {
    // Лаяут для юниформ буффера
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    memset(&uboLayoutBinding, 0, sizeof(VkDescriptorSetLayoutBinding));
    uboLayoutBinding.binding = 0;   // Юниформ буффер находится на 0 позиции
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Тип - юниформ буффер
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Предназначен буффер толкьо для вершинного шейдера, можно подрубить другие
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    // Лаяут для семплера
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    memset(&samplerLayoutBinding, 0, sizeof(VkDescriptorSetLayoutBinding));
    samplerLayoutBinding.binding = 1;   // Находится на 1й позиции
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;    // Семплер для текстуры
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Предназначено для фрагментного шейдера
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    // Биндинги
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {{uboLayoutBinding, samplerLayoutBinding}};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t >(bindings.size());
    layoutInfo.pBindings = bindings.data();

    // Создаем лаяут для шейдера
    VkResult status = vkCreateDescriptorSetLayout(vulkanDevice->vulkanLogicalDevice, &layoutInfo, nullptr, &vulkanDescriptorSetLayout);
    if (status != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

// Из байткода исходника создаем шейдерный модуль
void VulkanRenderInfo::createShaderModule(const std::vector<unsigned char>& code, VkShaderModule& shaderModule) {
    VkShaderModuleCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkShaderModuleCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = (uint32_t*)code.data();

    if (vkCreateShaderModule(vulkanDevice->vulkanLogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        LOGE("Failed to create shader module!");
        throw std::runtime_error("Failed to create shader module!");
    }
}

// Создание пайплайна отрисовки
void VulkanRenderInfo::createGraphicsPipeline() {
    // Уничтожаем старое
    if(vulkanPipeline != VK_NULL_HANDLE){
        vkDestroyPipeline(vulkanDevice->vulkanLogicalDevice, vulkanPipeline, nullptr);
        vulkanPipeline = VK_NULL_HANDLE;
    }
    if(vulkanPipelineLayout != VK_NULL_HANDLE){
        vkDestroyPipelineLayout(vulkanDevice->vulkanLogicalDevice, vulkanPipelineLayout, nullptr);
        vulkanPipelineLayout = VK_NULL_HANDLE;
    }

    // Читаем байт-код шейдеров
    std::vector<unsigned char> vertShaderCode = readFile(androidAssetManager, "shaders/vert.spv");
    std::vector<unsigned char> fragShaderCode = readFile(androidAssetManager, "shaders/frag.spv");

    // Создаем шейдерный модуль
    createShaderModule(vertShaderCode, vulkanVertexShader);
    createShaderModule(fragShaderCode, vulkanFragmentShader);

    // Удаляем данные
    vertShaderCode.clear();
    fragShaderCode.clear();

    // Описание настроек вершинного шейдера
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    memset(&vertShaderStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Вершинный шейдер
    vertShaderStageInfo.module = vulkanVertexShader;    // Модуль
    vertShaderStageInfo.pName = "main";     // Входная функция

    // Описание настроек фрагментного шейдера
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    memset(&fragShaderStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Фрагментный шейдер
    fragShaderStageInfo.module = vulkanFragmentShader;  // Модуль
    fragShaderStageInfo.pName = "main";     // Входная функция

    // Описание настроек глубины и трафарета у пайплайна
    // Поля depthBoundsTestEnable, minDepthBounds и maxDepthBounds используют для дополнительного теста связанной глубины.
    // По сути, это позволяет сохранить фрагменты, которые находятся в пределах заданного диапазона глубины, нам не понадобится.
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    memset(&depthStencil, 0, sizeof(VkPipelineDepthStencilStateCreateInfo));
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;     // Тест глубины включен
    depthStencil.depthWriteEnable = VK_TRUE;    // Запись глубины включена
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;   // Функция глубины
    depthStencil.depthBoundsTestEnable = VK_FALSE;  // TODO: ???
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Нужно для трафарета
    depthStencil.back = {};  // Нужно для трафарета

    // Описание вершин, шага по вершинам и описание данных
    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();

    // Описание формата входных данных
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    memset(&vertexInputInfo, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Топология вершин
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    memset(&inputAssembly, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;   // Рисуем обычными треугольниками
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Настраиваем вьюпорт
    VkViewport viewport = {};
    memset(&viewport, 0, sizeof(VkViewport));
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkanVisualizer->vulkanSwapChainExtent.width;
    viewport.height = (float)vulkanVisualizer->vulkanSwapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Выставляем сциссор
    VkRect2D scissor = {};
    memset(&scissor, 0, sizeof(VkRect2D));
    scissor.offset = {0, 0};
    scissor.extent = vulkanVisualizer->vulkanSwapChainExtent;

    // Создаем структуру настроек вьюпорта
    VkPipelineViewportStateCreateInfo viewportState = {};
    memset(&viewportState, 0, sizeof(VkPipelineViewportStateCreateInfo));
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Настройки растеризатора
    //  - Если depthClampEnable установлен в значение VK_TRUE, тогда фрагменты, находящиеся за ближней и дальней плоскостью, прикрепляются к ним, а не отбрасываются.
    // Это бывает полезно в ряде случаев, например для карты теней. Для использования необходимо включить функцию GPU.
    // - Если rasterizerDiscardEnable установлен в значение VK_TRUE, тогда геометрия не проходит через растеризатор.
    // По сути это отключает любой вывод в фреймбуфер.
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    memset(&rasterizer, 0, sizeof(VkPipelineRasterizationStateCreateInfo));
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;         // Включено ли округление глубины крайними значениями
    rasterizer.rasterizerDiscardEnable = VK_FALSE;  // Графика рисуется в буффер кадра
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Заполненные полигоны
    rasterizer.lineWidth = 1.0f;                    // Толщина линии
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;    //  Задняя часть
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Обход по часовой стрелке для фронтальной стороны
    rasterizer.depthBiasEnable = VK_FALSE;          // Смещение по глубине отключено

    // Настройка антиаллиасинга с помощью мультисемплинга
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    memset(&multisampling, 0, sizeof(VkPipelineMultisampleStateCreateInfo));
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;    // Кратность семплирования
    multisampling.minSampleShading = 1.0f;      // Optional
    multisampling.pSampleMask = nullptr;        // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    // Настройки классического блендинга
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    memset(&colorBlendAttachment, 0, sizeof(VkPipelineColorBlendAttachmentState));
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;    // Цвета, которые пишем
    colorBlendAttachment.blendEnable = VK_FALSE;    // Блендинг выключен
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // Настройка конкретного блендинга
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    memset(&colorBlending, 0, sizeof(VkPipelineColorBlendStateCreateInfo));
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Лаяут пайплайна
    VkDescriptorSetLayout setLayouts[] = {vulkanDescriptorSetLayout};   // Лаяют для юниформ буффер и семплера
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    memset(&pipelineLayoutInfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts; // Устанавливаем лаяут
    pipelineLayoutInfo.pushConstantRangeCount = 0;  // TODO: Пуш-константы??
    pipelineLayoutInfo.pPushConstantRanges = 0;
    // Пуш константы нужны для того, чтобы передавать данные в отрисовку, как альтернатива юниформам, но без изменения??

    if (vkCreatePipelineLayout(vulkanDevice->vulkanLogicalDevice, &pipelineLayoutInfo, nullptr, &vulkanPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // TODO: Наследование позволяет ускорить переключение пайплайнов с общим родителем

    // Непосредственно создание пайплайна
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    memset(&pipelineInfo, 0, sizeof(VkGraphicsPipelineCreateInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;            // 2 стадии, вершинный и фрагментный шейдер
    pipelineInfo.pStages = shaderStages;    // Шейдеры
    pipelineInfo.pVertexInputState = &vertexInputInfo;  // Описание входных вершин
    pipelineInfo.pInputAssemblyState = &inputAssembly;  // Топология вершин (Рисуем треугольниками)
    pipelineInfo.pViewportState = &viewportState;       // Вьюпорт и scissor отрисовки
    pipelineInfo.pRasterizationState = &rasterizer;     // Растеризатор (кулинг сторон и режим полигона)
    pipelineInfo.pDepthStencilState = &depthStencil;    // Настройки работы с глубиной
    pipelineInfo.pMultisampleState = &multisampling;    // Настройки семплирования для антиалиассинга
    pipelineInfo.pColorBlendState = &colorBlending;     // Настройка смешивания цветов
    pipelineInfo.layout = vulkanPipelineLayout;         // Лаяут пайплайна (Описание буфферов юниформов и семплеров)
    pipelineInfo.renderPass = vulkanRenderPass;         // Рендер-проход
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;   // Родительский пайплайн
    pipelineInfo.pDynamicState = nullptr;               // Динамическое состояние отрисовки

    if (vkCreateGraphicsPipelines(vulkanDevice->vulkanLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vulkanPipeline) != VK_SUCCESS) {
        LOGE("Failed to create graphics pipeline!");
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

// Создаем пулл комманд
void VulkanRenderInfo::createRenderCommandPool() {
    // Информация о пуле коммандных буфферов
    VkCommandPoolCreateInfo poolInfo = {};
    memset(&poolInfo, 0, sizeof(VkCommandPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(vulkanDevice->vulkanFamiliesQueueIndexes.presentQueueFamilyIndex);   // Пулл будет для семейства очередей рендеринга
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(vulkanDevice->vulkanLogicalDevice, &poolInfo, nullptr, &vulkanCommandPool) != VK_SUCCESS) {
        LOGE("Failed to create command pool!");
        throw std::runtime_error("Failed to create command pool!");
    }
}

// Обновляем лаяут текстуры глубины на правильный
void VulkanRenderInfo::updateDepthTextureLayout(){
    // Конвертируем в формат, пригодный для глубины
    transitionImageLayout(vulkanVisualizer->vulkanDepthImage,
                          vulkanVisualizer->vulkanDepthFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

// Запуск коммандного буффера на получение комманд
VkCommandBuffer VulkanRenderInfo::beginSingleTimeCommands() {
    // Параметр level определяет, будет ли выделенный буфер команд первичным или вторичным буфером команд:
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Может быть передан очереди для исполнения, но не может быть вызван из других буферов команд.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: не может быть передан непосредственно, но может быть вызван из первичных буферов команд.
    VkCommandBufferAllocateInfo allocInfo = {};
    memset(&allocInfo, 0, sizeof(VkCommandBufferAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Первичный буффер, которыый будет исполняться сразу
    allocInfo.commandPool = vulkanCommandPool;      // Пул комманд
    allocInfo.commandBufferCount = 1;

    // Аллоцируем коммандный буффер для задач, который будут закидываться в очередь
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkanDevice->vulkanLogicalDevice, &allocInfo, &commandBuffer);

    // Параметр flags определяет, как использовать буфер команд. Возможны следующие значения:
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: Буфер команд будет перезаписан сразу после первого выполнения.
    // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: Это вторичный буфер команд, который будет в единственном render pass.
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: Буфер команд может быть представлен еще раз, если он так же уже находится в ожидании исполнения.

    // Настройки запуска коммандного буффера
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Запускаем буффер комманд
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

// Завершение коммандного буффера
void VulkanRenderInfo::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // Заканчиваем прием комманд
    vkEndCommandBuffer(commandBuffer);

    // Структура с описанием отправки в буффер
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Отправляем задание на отрисовку в буффер отрисовки
    vkQueueSubmit(vulkanDevice->vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    // TODO: Ожидание передачи комманды в очередь на GPU???
    vkQueueWaitIdle(vulkanDevice->vulkanGraphicsQueue);
    /*std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
    vkQueueWaitIdle(vulkanGraphicsQueue);
    std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration elapsed = time2 - time1;
    int64_t elapsedMicroSec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    printf("Wait duration (vkQueueWaitIdle(vulkanGraphicsQueue)): %lldmicroSec\n", elapsedMicroSec);
    fflush(stdout);*/

    // Удаляем коммандный буффер
    vkFreeCommandBuffers(vulkanDevice->vulkanLogicalDevice, vulkanCommandPool, 1, &commandBuffer);
}

// Перевод изображения из одного лаяута в другой (из одного способа использования в другой)
void VulkanRenderInfo::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    // Создаем коммандный буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Создаем барьер памяти для картинок
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;  // Старый лаяут (способ использования)
    barrier.newLayout = newLayout;  // Новый лаяут (способ использования)
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // Очередь не меняется
    barrier.image = image;  // Изображение, которое меняется
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;    // Изображение использовалось для цвета
    barrier.subresourceRange.baseMipLevel = 0;  // 0 левел мипмапов
    barrier.subresourceRange.levelCount = 1;    // 1 уровень мипмапов
    barrier.subresourceRange.baseArrayLayer = 0;    // Базовый уровень
    barrier.subresourceRange.layerCount = 1;        // 1 уровень

    // Настраиваем условия ожиданий для конвертации
    // TODO: ???
    if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;       // Ожидание записи хостом данных
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;    // TODO: Дальнейшие действия возможнны после чтения из текстуры???
    } else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        printf("Unsupported layout transition!");
        fflush(stdout);
        throw std::invalid_argument("Unsupported layout transition!");
    }

    // TODO: ???
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // Закидываем в очередь барьер конвертации использования для изображения
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Закидываем на верх пайплайна
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Закидываем на верх пайплайна
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    endSingleTimeCommands(commandBuffer);
}


// Закидываем в очередь операцию копирования текстуры
void VulkanRenderInfo::queueCopyImage(VkImage srcImage, VkImage dstImage, uint32_t width,
                                      uint32_t height) {
    // TODO: Надо ли для группы операций с текстурами каждый раз создавать коммандный буффер?? Может быть можно все делать в одном?
    // Создаем коммандный буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Описание ресурса
    VkImageSubresourceLayers subResource = {};
    memset(&subResource, 0, sizeof(VkImageSubresourceLayers));
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Текстура с цветом
    subResource.layerCount = 1; // Всего 1н слой
    subResource.baseArrayLayer = 0; // 0й слой
    subResource.mipLevel = 0;   // 0й уровень мипмаппинга

    // Регион копирования текстуры
    VkImageCopy region = {};
    memset(&region, 0, sizeof(VkImageCopy));
    region.srcSubresource = subResource;
    region.dstSubresource = subResource;
    region.srcOffset = {0, 0, 0};
    region.dstOffset = {0, 0, 0};
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;

    // Создаем задачу на копирование данных
    vkCmdCopyImage(commandBuffer,
                   srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);

    // Завершаем буффер комманд
    endSingleTimeCommands(commandBuffer);
}

// Копирование буффера
void VulkanRenderInfo::queueCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // Запускаем буффер
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Ставим в очередь копирование буффера
    VkBufferCopy copyRegion = {};
    memset(&copyRegion, 0, sizeof(VkBufferCopy));
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    // Заканчиваем буффер
    endSingleTimeCommands(commandBuffer);
}