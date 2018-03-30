// Windows
#ifdef _MSVC_LANG 
    #define NOMINMAX
    #include <windows.h>
#endif

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <algorithm>
#include <limits>

// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// STB image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// TinyObj
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "VulkanRender.h"
#include "CommonDefines.h"
#include "CommonConstants.h"
#include "Vertex.h"
#include "Figures.h"
#include "UniformBuffer.h"
#include "VulkanHelpers.h"


#define APPLICATION_SAMPLING_VALUE VK_SAMPLE_COUNT_1_BIT

struct FamiliesQueueIndexes;


GLFWwindow* window = nullptr;



VkFence vulkanFence = VK_NULL_HANDLE;


std::vector<VkCommandBuffer> vulkanCommandBuffers;
VkDescriptorSet vulkanDescriptorSet = VK_NULL_HANDLE;

////////////////////////////////////////////////////////////////////////////////////////////////

float rotateAngle = 0.0f;



// Создаем преграды для проверки завершения комманд отрисовки
void createFences(){
    VkFenceCreateInfo createInfo = {};
    memset(&createInfo, 0, sizeof(VkFenceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Изначально создаем вытавленным
    VkResult fenceCreateStatus = vkCreateFence(RenderI->vulkanLogicalDevice->getDevice(), &createInfo, nullptr, &vulkanFence);
    if (fenceCreateStatus != VK_SUCCESS) {
        printf("Failed to create fence!");
        fflush(stdout);
        throw std::runtime_error("Failed to create fence!");
    }
}



// Вызывается при различных ресайзах окна
void recreateSwapChain() {
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(RenderI->vulkanRenderQueue->getQueue());
    vkQueueWaitIdle(RenderI->vulkanPresentQueue->getQueue());
    vkDeviceWaitIdle(RenderI->vulkanLogicalDevice->getDevice());
    
    // Заново пересоздаем свопчейны, старые удалятся внутри
    //createSwapChain();
    //getSwapchainImages();
    //createImageViews();
    //createDepthResources();
    //updateDepthTextureLayout();
    //createRenderPass();
    //createFramebuffers();
    //createGraphicsPipeline();
    //createCommandBuffers();
}



// Непосредственно отрисовка кадра
void drawFrame() {
    // Запрашиваем изображение для отображения из swapchain, время ожидания делаем максимальным
    uint32_t swapchainImageIndex = 0;    // Индекс картинки свопчейна
    VkResult result = vkAcquireNextImageKHR(RenderI->vulkanLogicalDevice->getDevice(), RenderI->vulkanSwapchain->getSwapchain(),
                                            std::numeric_limits<uint64_t>::max(),
                                            RenderI->vulkanImageAvailableSemaphore->getSemafore(), // Семафор ожидания доступной картинки
                                            VK_NULL_HANDLE,
                                            &swapchainImageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    // Проверяем, совпадает ли номер картинки и индекс картинки свопчейна
    if (vulkanImageIndex != swapchainImageIndex) {
        printf("Vulkan image index not equal to swapchain image index!\n");
        fflush(stdout);
    }
    
    // Настраиваем отправление в очередь комманд отрисовки
    // http://vulkanapi.ru/2016/11/14/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-29-%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D0%B8%D0%BD%D0%B3-%D0%B8-%D0%BF%D1%80%D0%B5%D0%B4%D1%81%D1%82%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5-hello-wo/
    VkSemaphore waitSemaphores[] = {RenderI->vulkanImageAvailableSemaphore->getSemafore()}; // Семафор ожидания картинки для вывода туда графики
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};    // Ждать будем c помощью семафора возможности вывода в буфер цвета
    VkSemaphore signalSemaphores[] = {RenderI->vulkanRenderFinishedSemaphore->getSemafore()}; // Семафор оповещения о завершении рендеринга
    VkSubmitInfo submitInfo = {};
    memset(&submitInfo, 0, sizeof(VkSubmitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;    // Ожидаем доступное изображение, в которое можно было бы записывать пиксели
    submitInfo.pWaitDstStageMask = waitStages;      // Ждать будем возможности вывода в буфер цвета
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanCommandBuffers[vulkanImageIndex]; // Указываем коммандный буффер отрисовки
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    // Синхронизация с ожиданием на CPU завершения очереди выполнения комманд
    /*
    //VkResult fenceStatus = vkGetFenceStatus(vulkanLogicalDevice, vulkanFence);
    //VkResult resetFenceStatus = vkResetFences(vulkanLogicalDevice, 1, &vulkanFence);
    VkResult waitStatus = vkWaitForFences(vulkanLogicalDevice, 1, &vulkanFence, VK_TRUE, std::numeric_limits<uint64_t>::max()-1);
    if (waitStatus == VK_SUCCESS) {
        VkResult resetFenceStatus = vkResetFences(vulkanLogicalDevice, 1, &vulkanFence);
        //printf("Fence waited + reset!\n");
        //fflush(stdout);
    }*/
    
    // Кидаем в очередь задачу на отрисовку с указанным коммандным буффером
    if (vkQueueSubmit(RenderI->vulkanRenderQueue->getQueue(), 1, &submitInfo,  VK_NULL_HANDLE/*vulkanFence*/) != VK_SUCCESS) {
        printf("Failed to submit draw command buffer!\n");
        fflush(stdout);
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    
    // Можно не получать индекс, а просто делать как в Metal, либо на всякий случай получить индекс на старте
    vulkanImageIndex = (vulkanImageIndex + 1) % RenderI->vulkanSwapchain->getImageViews().size();
    
    // Настраиваем задачу отображения полученного изображения
    VkSwapchainKHR swapChains[] = {RenderI->vulkanSwapchain->getSwapchain()};
    VkPresentInfoKHR presentInfo = {};
    memset(&presentInfo, 0, sizeof(VkPresentInfoKHR));
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Ожидаем окончания подготовки кадра с помощью семафора
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &swapchainImageIndex;
    
    // Закидываем в очередь задачу отображения картинки
    VkResult presentResult = vkQueuePresentKHR(RenderI->vulkanPresentQueue->getQueue(), &presentInfo);
    
    // В случае проблем - пересоздаем свопчейн
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        printf("failed to present swap chain image!\n");
        throw std::runtime_error("failed to present swap chain image!");
    }
}

// Коллбек, вызываемый при изменении размеров окна приложения
void onGLFWWindowResized(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    recreateSwapChain();
}

#ifndef _MSVC_LANG
int main(int argc, char** argv) {
#else
int local_main(int argc, char** argv) {
#endif
    glfwInit();
    
    // Говорим GLFW, что не нужно создавать GL контекст
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Окно без изменения размера
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // Мультисемплинг
    //glfwWindowHint(GLFW_SAMPLES, 4);
    
    // Создаем окно
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onGLFWWindowResized);
    
    // Проверяем наличие поддержки Vulkan
    int vulkanSupportStatus = glfwVulkanSupported();
    if (vulkanSupportStatus != GLFW_TRUE){
        printf("Vulkan support not found, error 0x%08x\n", vulkanSupportStatus);
		fflush(stdout);
        throw std::runtime_error("Vulkan support not found!");
    }

    // Создаем рендер
    VulkanRender::initInstance(window);
    
    // Создаем преграды для проверки завершения комманд отрисовки
    createFences();
    
    
    // Цикл обработки графики
    std::chrono::high_resolution_clock::time_point lastDrawTime = std::chrono::high_resolution_clock::now();
    double lastFrameDuration = 1.0/60.0;
    int totalFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Обновляем юниформы
        updateUniformBuffer(lastFrameDuration);
        
        // Непосредственно отрисовка кадра
        drawFrame();
        
        // Стабилизация времени кадра
        std::chrono::high_resolution_clock::duration curFrameDuration = std::chrono::high_resolution_clock::now() - lastDrawTime;
        std::chrono::high_resolution_clock::duration sleepDuration = std::chrono::milliseconds(static_cast<int>(1.0/60.0 * 1000.0)) - curFrameDuration;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count() > 0) {
            std::this_thread::sleep_for(sleepDuration);
        }
        // Расчет времени кадра
        lastFrameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastDrawTime).count() / 1000.0;
        lastDrawTime = std::chrono::high_resolution_clock::now(); // TODO: Возможно - правильнее было бы перетащить в начало цикла
        
        // FPS
        totalFrames++;
        if (totalFrames > 60) {
            totalFrames = 0;
            char outText[64];
            sprintf(outText, "Possible FPS: %d, sleep duration: %lldms", static_cast<int>(1.0/lastFrameDuration), std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count());
            glfwSetWindowTitle(window, outText);
        }
   }
    
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(RenderI->vulkanRenderQueue->getQueue());
    vkQueueWaitIdle(RenderI->vulkanPresentQueue->getQueue());
    vkDeviceWaitIdle(RenderI->vulkanLogicalDevice->getDevice());

    
    vkDestroyFence(RenderI->vulkanLogicalDevice->getDevice(), vulkanFence, nullptr);
        
    VulkanRender::destroyRender();
    
    // Очищаем GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}

#ifdef _MSVC_LANG
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	local_main(0, NULL);
	return 0;
}
#endif

