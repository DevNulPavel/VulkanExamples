#ifndef VERTEX_H
#define VERTEX_H

#include <cstring>
#include <array>

// Vulkan
#include <vulkan_wrapper.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    
    // Описание размерности вершины
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        memset(&bindingDescription, 0, sizeof(VkVertexInputBindingDescription));
        bindingDescription.binding = 0;     // Буффер вершинной информации находится на 0 индексе
        bindingDescription.stride = sizeof(Vertex); // Размер шага равен размеру данной вершины
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Идем повершинно
        return bindingDescription;
    }
    
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
        // Позиция
        memset(&attributeDescriptions[0], 0, sizeof(VkVertexInputAttributeDescription));
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;  // 0 в шейдере
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // Цвет
        memset(&attributeDescriptions[1], 0, sizeof(VkVertexInputAttributeDescription));
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;  // 1 в шейдере
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        // TexCoord
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2; // 2 в шейдере
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        
        return attributeDescriptions;
    }
};

#endif
