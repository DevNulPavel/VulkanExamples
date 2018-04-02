#ifndef VERTEX2D_H
#define VERTEX2D_H

#include <cstring>
#include <vector>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>


struct Vertex2D {
    glm::vec2 pos;
    glm::vec2 texCoord;
    
    // Описание размерности вершины
    static VkVertexInputBindingDescription getBindingDescription();
    
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

#endif
