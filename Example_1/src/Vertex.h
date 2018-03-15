#ifndef VERTEX_H
#define VERTEX_H

#include <array>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    
    // Описание размерности вершины
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
        // Позиция
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;  // 0 в шейдере
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // Цвет
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;  // 1 в шейдере
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};

#endif
