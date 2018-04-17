#ifndef VULKAN_REFLECTION_H
#define VULKAN_REFLECTION_H

#include <vector>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Запускаем рефлексию шейдера
void reflectShaderUsingSPIRVCross(const std::vector<unsigned char>& data);

void reflectShaderUsingSPIRVReflect(const std::vector<unsigned char>& data);

#endif
