#include "Vertex2D.h"


// Описание размерности вершины
VkVertexInputBindingDescription Vertex2D::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    memset(&bindingDescription, 0, sizeof(VkVertexInputBindingDescription));
    bindingDescription.binding = 0;     // Буффер вершинной информации находится на 0 индексе
    bindingDescription.stride = sizeof(Vertex2D); // Размер шага равен размеру данной вершины
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Идем повершинно
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex2D::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.resize(2);
    
    // Позиция
    memset(&attributeDescriptions[0], 0, sizeof(VkVertexInputAttributeDescription));
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;  // 0 в шейдере
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex2D, pos);
    // TexCoord
    memset(&attributeDescriptions[1], 0, sizeof(VkVertexInputAttributeDescription));
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1; // 1 в шейдере
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex2D, texCoord);
    
    return attributeDescriptions;
    }
