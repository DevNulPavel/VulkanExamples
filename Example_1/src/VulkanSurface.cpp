#include "VulkanSurface.h"
#include <cstdio>
#include <stdexcept>
#include "CommonConstants.h"


VulkanSurface::VulkanSurface(GLFWwindow* window, VulkanInstancePtr instance):
    _surface(VK_NULL_HANDLE),
    _instance(instance){
        
    if (glfwCreateWindowSurface(_instance->getInstance(), window, nullptr, &_surface) != VK_SUCCESS) {
        printf("Failed to create window surface!");
        fflush(stdout);
        throw std::runtime_error("Failed to create window surface!");
    }
}

VulkanSurface::~VulkanSurface(){
    vkDestroySurfaceKHR(_instance->getInstance(), _surface, nullptr);
}

VkSurfaceKHR VulkanSurface::getSurface() const{
    return _surface;
}

VulkanInstancePtr VulkanSurface::getBaseInstance() const{
    return _instance;
}
