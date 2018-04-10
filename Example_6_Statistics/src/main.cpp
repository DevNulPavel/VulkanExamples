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

#include "VulkanRender.h"
#include "CommonDefines.h"
#include "CommonConstants.h"
#include "Vertex.h"
#include "Figures.h"
#include "UniformBuffer.h"
#include "VulkanHelpers.h"
#include "Helpers.h"


GLFWwindow* window = nullptr;

// Коллбек, вызываемый при изменении размеров окна приложения
void onGLFWWindowResized(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    
    RenderI->windowResized(window, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
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
	// Frame rate
	glfwWindowHint(GLFW_REFRESH_RATE, 60);
    
    // Создаем окно
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, onGLFWWindowResized);
    
    // Проверяем наличие поддержки Vulkan
    int vulkanSupportStatus = glfwVulkanSupported();
    if (vulkanSupportStatus != GLFW_TRUE){
        LOG("Vulkan support not found, error 0x%08x\n", vulkanSupportStatus);
        throw std::runtime_error("Vulkan support not found!");
    }

    // Создаем рендер
    VulkanRender::initInstance(window);    
    
    // Цикл обработки графики
    std::chrono::high_resolution_clock::time_point lastDrawTime = std::chrono::high_resolution_clock::now();
    double lastFrameDuration = 1.0/60.0;
    int totalFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        std::chrono::high_resolution_clock::time_point drawBegin = std::chrono::high_resolution_clock::now();
        
        // Обновляем юниформы
        VulkanRender::getInstance()->updateRender(lastFrameDuration);
        
        // Непосредственно отрисовка кадра
        VulkanRender::getInstance()->drawFrame();
        
        // Стабилизация времени кадра
        std::chrono::high_resolution_clock::duration drawCallDuration = std::chrono::high_resolution_clock::now() - drawBegin;
        std::chrono::high_resolution_clock::duration sleepDuration = std::chrono::milliseconds(static_cast<int>(1.0/60.0 * 1000.0)) - drawCallDuration;
        
        //TIME_BEGIN(SLEEP_TEST);
        if (std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count() > 0) {
            // Низкая точность на винде
            sleepShort(std::chrono::duration_cast<std::chrono::microseconds>(sleepDuration).count() / 1000.0f);
        }
        //TIME_END_MICROSEC(SLEEP_TEST, "Sleep time");
        
        // Расчет времени кадра
        lastFrameDuration = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - lastDrawTime).count() / 1000.0 / 1000.0;
        lastDrawTime = std::chrono::high_resolution_clock::now(); // TODO: Возможно - правильнее было бы перетащить в начало цикла
        
        // FPS
        totalFrames++;
        if (totalFrames >= 30) {
            totalFrames = 0;
            char outText[128];
            sprintf(outText, "Possible FPS: %d (%.1fms), CPU draw duration %.1fms, sleep duration: %.1fms",
                    static_cast<int>(1.0/lastFrameDuration), lastFrameDuration*1000.0,
                    (double)std::chrono::duration_cast<std::chrono::microseconds>(drawCallDuration).count() / 1000.0,
                    (double)std::chrono::duration_cast<std::chrono::microseconds>(sleepDuration).count() / 1000.0 );
            glfwSetWindowTitle(window, outText);
        }
   }
        
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

