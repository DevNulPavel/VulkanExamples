#include "VulkanSurface.h"
#include <cstdio>
#include <stdexcept>
#include "CommonConstants.h"
#include "VulkanRender.h"


VulkanSurface::VulkanSurface(GLFWwindow* window, VulkanInstancePtr instance):
    surface(VK_NULL_HANDLE),
    _instance(instance){
        
    createDrawSurface(window);
}

VulkanSurface::~VulkanSurface(){
    vkDestroySurfaceKHR(RenderI->vulkanInstance->instance, surface, nullptr);
}

// Создаем плоскость отрисовки GLFW
void VulkanSurface::createDrawSurface(GLFWwindow* window) {
    if (glfwCreateWindowSurface(_instance->instance, window, nullptr, &surface) != VK_SUCCESS) {
        printf("Failed to create window surface!");
        fflush(stdout);
        throw std::runtime_error("Failed to create window surface!");
    }
}
