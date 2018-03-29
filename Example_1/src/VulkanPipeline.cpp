#include "VulkanPipeline.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>



VulkanPipelineDepthConfig::VulkanPipelineDepthConfig():
    depthTestEnabled(VK_FALSE),
    depthWriteEnabled(VK_FALSE),
    depthFunc(VK_COMPARE_OP_LESS){
}

VulkanPipelineCullingConfig::VulkanPipelineCullingConfig():
    cullMode(VK_CULL_MODE_NONE),
    frontFace(VK_FRONT_FACE_CLOCKWISE){
}

VulkanPipelineBlendConfig::VulkanPipelineBlendConfig():
    enabled(VK_FALSE),
    srcFactor(VK_BLEND_FACTOR_SRC_ALPHA),
    dstFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA),
    blendOp(VK_BLEND_OP_ADD){
}

VulkanPipeline::VulkanPipeline(VulkanLogicalDevicePtr device,
                               VulkanShaderModulePtr vertexShader, VulkanShaderModulePtr fragmentShader,
                               VulkanPipelineDepthConfig depthConfig,
                               VkVertexInputBindingDescription vertexBindingDescription,
                               std::vector<VkVertexInputAttributeDescription> vertexAttributesDescriptions,
                               VkPrimitiveTopology primitivesTypes,
                               VkViewport viewport,
                               VkRect2D scissor,
                               VulkanPipelineCullingConfig cullingConfig,
                               VulkanPipelineBlendConfig blendConfig,
                               VulkanDescriptorSetLayoutPtr descriptorSetLayout,
                               VulkanRenderPassPtr renderPass):
    _device(device),
    _vertexShader(vertexShader),
    _fragmentShader(fragmentShader),
    _depthConfig(depthConfig),
    _vertexBindingDescription(vertexBindingDescription),
    _vertexAttributesDescriptions(vertexAttributesDescriptions),
    _primitivesTypes(primitivesTypes),
    _viewport(viewport),
    _scissor(scissor),
    _cullingConfig(cullingConfig),
    _blendConfig(blendConfig),
    _descriptorSetLayout(descriptorSetLayout),
    _renderPass(renderPass){
        
    // Описание настроек вершинного шейдера
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    memset(&vertShaderStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Вершинный шейдер
    vertShaderStageInfo.module = _vertexShader->getModule();    // Модуль
    vertShaderStageInfo.pName = "main";     // Входная функция
    
    // Описание настроек фрагментного шейдера
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    memset(&fragShaderStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Фрагментный шейдер
    fragShaderStageInfo.module = _fragmentShader->getModule();  // Модуль
    fragShaderStageInfo.pName = "main";     // Входная функция
        
    // Описание настроек глубины и трафарета у пайплайна
    // Поля depthBoundsTestEnable, minDepthBounds и maxDepthBounds используют для дополнительного теста связанной глубины.
    // По сути, это позволяет сохранить фрагменты, которые находятся в пределах заданного диапазона глубины, нам не понадобится.
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    memset(&depthStencil, 0, sizeof(VkPipelineDepthStencilStateCreateInfo));
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = _depthConfig.depthTestEnabled;     // Тест глубины включен
    depthStencil.depthWriteEnable = _depthConfig.depthWriteEnabled;    // Запись глубины включена
    depthStencil.depthCompareOp = _depthConfig.depthFunc;   // Функция глубины
    depthStencil.depthBoundsTestEnable = VK_FALSE;  // TODO: ???
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Нужно для трафарета
    depthStencil.back = {};  // Нужно для трафарета
        
    // Описание формата входных данных
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    memset(&vertexInputInfo, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &_vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(_vertexAttributesDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = _vertexAttributesDescriptions.data();
        
    // Топология вершин
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    memset(&inputAssembly, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = _primitivesTypes;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
        
    // Создаем структуру настроек вьюпорта
    VkPipelineViewportStateCreateInfo viewportState = {};
    memset(&viewportState, 0, sizeof(VkPipelineViewportStateCreateInfo));
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &_viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &_scissor;
        
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
    rasterizer.cullMode = _cullingConfig.cullMode;    // Задняя часть
    rasterizer.frontFace = _cullingConfig.frontFace; // Обход по часовой стрелке для фронтальной стороны
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
    colorBlendAttachment.blendEnable = _blendConfig.enabled;    // Блендинг выключен
    colorBlendAttachment.srcColorBlendFactor = _blendConfig.srcFactor;
    colorBlendAttachment.dstColorBlendFactor = _blendConfig.dstFactor;
    colorBlendAttachment.colorBlendOp = _blendConfig.blendOp;
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
    VkDescriptorSetLayout setLayouts[] = {_descriptorSetLayout->getLayout()};   // Лаяют для юниформ буффер и семплера
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    memset(&pipelineLayoutInfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts; // Устанавливаем лаяут
    pipelineLayoutInfo.pushConstantRangeCount = 0;  // TODO: Пуш-константы??
    pipelineLayoutInfo.pPushConstantRanges = 0;
    // Пуш константы нужны для того, чтобы передавать данные в отрисовку, как альтернатива юниформам, но без изменения??
    
    if (vkCreatePipelineLayout(_device->getDevice(), &pipelineLayoutInfo, nullptr, &_layout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to create pipeline layout!");
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
    pipelineInfo.layout = _layout;                      // Лаяут пайплайна (Описание буфферов юниформов и семплеров)
    pipelineInfo.renderPass = _renderPass->getPass();         // Рендер-проход
    pipelineInfo.subpass = 0;   // Для какого
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;   // Родительский пайплайн
    pipelineInfo.pDynamicState = nullptr;               // Динамическое состояние отрисовки
    
    if (vkCreateGraphicsPipelines(_device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

VulkanPipeline::~VulkanPipeline(){
    vkDestroyPipelineLayout(_device->getDevice(), _layout, nullptr);
    vkDestroyPipeline(_device->getDevice(), _pipeline, nullptr);
}

VkPipelineLayout VulkanPipeline::getLayout() const{
    return _layout;
}

VkPipeline VulkanPipeline::getPipeline() const{
    return _pipeline;
}

