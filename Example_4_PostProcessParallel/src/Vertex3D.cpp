#include "Vertex3D.h"


VkVertexInputBindingDescription Vertex3D::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    memset(&bindingDescription, 0, sizeof(VkVertexInputBindingDescription));
    bindingDescription.binding = 0;     // Буффер вершинной информации находится на 0 индексе
    bindingDescription.stride = sizeof(Vertex3D); // Размер шага равен размеру данной вершины
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Идем повершинно
    return bindingDescription;
}


std::vector<VkVertexInputAttributeDescription> Vertex3D::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.resize(3);
    
    // Позиция
    memset(&attributeDescriptions[0], 0, sizeof(VkVertexInputAttributeDescription));
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;  // 0 в шейдере
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex3D, pos);
    // Цвет
    memset(&attributeDescriptions[1], 0, sizeof(VkVertexInputAttributeDescription));
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;  // 1 в шейдере
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex3D, color);
    // TexCoord
    memset(&attributeDescriptions[2], 0, sizeof(VkVertexInputAttributeDescription));
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2; // 2 в шейдере
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex3D, texCoord);
    
    return attributeDescriptions;
}
