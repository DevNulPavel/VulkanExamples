#ifndef VULKAN_SURFACE_H
#define VULKAN_SURFACE_H

#include <vector>
#include <map>
#include <string>
#include <memory>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"


struct VulkanSurface {
public:
    VulkanSurface(GLFWwindow* window, VulkanInstancePtr instance);
    ~VulkanSurface();
    VkSurfaceKHR getSurface() const;
    
private:
    // Создаем плоскость отрисовки GLFW
    void createDrawSurface(GLFWwindow* window);

private:
    VkSurfaceKHR _surface;
    VulkanInstancePtr _instance;
};

typedef std::shared_ptr<VulkanSurface> VulkanSurfacePtr;

#endif
