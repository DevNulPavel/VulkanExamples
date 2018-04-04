#include "VulkanSurface.h"
#include <cstdio>
#include <stdexcept>
#include "CommonConstants.h"
#include "Helpers.h"


VulkanSurface::VulkanSurface(GLFWwindow* window, VulkanInstancePtr instance):
    _surface(VK_NULL_HANDLE),
    _instance(instance){
        
    if (glfwCreateWindowSurface(_instance->getInstance(), window, nullptr, &_surface) != VK_SUCCESS) {
        LOG("Failed to create window surface!");
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
