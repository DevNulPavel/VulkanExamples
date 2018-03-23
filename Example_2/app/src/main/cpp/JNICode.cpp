#include <jni.h>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <vulkan_wrapper.h>
#include "VulkanDevice.h"
#include "VulkanVisualizer.h"
#include "VulkanRenderInfo.h"
#include "VulkanModelInfo.h"
#include "SupportFunctions.h"

//JNIEXPORT jstring
//JNICALL


////////////////////////////////////////////////////////////////////////////////////
VulkanDevice* vulkanDevice = nullptr;
VulkanVisualizer* vulkanVisualizer = nullptr;
VulkanRenderInfo* vulkanRenderInfo = nullptr;
VulkanModelInfo* vulkanModelInfo = nullptr;
std::chrono::high_resolution_clock::time_point lastDrawTime = std::chrono::high_resolution_clock::now();
double lastFrameDuration = 1.0/60.0;
bool fistDraw = true;


////////////////////////////////////////////////////////////////////////////////////

// Непосредственно отрисовка кадра
void drawFrame() {
    // Запрашиваем изображение для отображения из swapchain, время ожидания делаем максимальным
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(vulkanDevice->vulkanLogicalDevice, vulkanVisualizer->vulkanSwapchain,
                                            std::numeric_limits<uint64_t>::max(),
                                            vulkanDevice->vulkanImageAvailableSemaphore, // Семафор ожидания доступной картинки
                                            VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        //recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // Настраиваем отправление в очередь комманд отрисовки
    // http://vulkanapi.ru/2016/11/14/vulkan-api-%D1%83%D1%80%D0%BE%D0%BA-29-%D1%80%D0%B5%D0%BD%D0%B4%D0%B5%D1%80%D0%B8%D0%BD%D0%B3-%D0%B8-%D0%BF%D1%80%D0%B5%D0%B4%D1%81%D1%82%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5-hello-wo/
    VkSemaphore waitSemaphores[] = {vulkanDevice->vulkanImageAvailableSemaphore}; // Семафор ожидания картинки для вывода туда графики
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};    // Ждать будем возможности вывода в буфер цвета
    VkSemaphore signalSemaphores[] = {vulkanDevice->vulkanRenderFinishedSemaphore}; // Семафор оповещения о завершении рендеринга
    VkSubmitInfo submitInfo = {};
    memset(&submitInfo, 0, sizeof(VkSubmitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;    // Ожидаем доступное изображение, в которое можно было бы записывать пиксели
    submitInfo.pWaitDstStageMask = waitStages;      // Ждать будем возможности вывода в буфер цвета
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(vulkanModelInfo->vulkanCommandBuffers[imageIndex]); // Указываем коммандный буффер отрисовки
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Кидаем в очередь задачу на отрисовку с указанным коммандным буффером
    if (vkQueueSubmit(vulkanDevice->vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Настраиваем задачу отображения полученного изображения
    VkSwapchainKHR swapChains[] = {vulkanVisualizer->vulkanSwapchain};
    VkPresentInfoKHR presentInfo = {};
    memset(&presentInfo, 0, sizeof(VkPresentInfoKHR));
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Ожидаем окончания подготовки кадра с помощью семафора
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    // Закидываем в очередь задачу отображения картинки
    VkResult presentResult = vkQueuePresentKHR(vulkanDevice->vulkanPresentQueue, &presentInfo);

    // В случае проблем - пересоздаем свопчейн
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        //recreateSwapChain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

extern "C" {

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanInit(JNIEnv *env,
                                                                       jobject thisObj,
                                                                       jobject surface,
                                                                       jint width, jint height,
                                                                       jobject assetManager) {
    // Инициализируем указатели на функции Vulkan
    int status = InitVulkan();
    if (status == 0){
        LOGE("Vulkan dynamic library loading failed!");
        throw std::runtime_error("Failed to InitVulkan!");
    }

    ANativeWindow* androidNativeWindow = ANativeWindow_fromSurface(env, surface);
    AAssetManager* androidNativeAssetManager = AAssetManager_fromJava(env, assetManager);

    // Создание устройства
    vulkanDevice = new VulkanDevice(androidNativeWindow, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    // Создание свопчейна
    vulkanVisualizer = new VulkanVisualizer(vulkanDevice);

    // Рендер инфо
    vulkanRenderInfo = new VulkanRenderInfo(vulkanDevice, vulkanVisualizer, androidNativeAssetManager);

    // Создаем фреймбуфферы для рендер-прохода
    vulkanVisualizer->createFramebuffers(vulkanRenderInfo);

    // Данные о модели
    vulkanModelInfo = new VulkanModelInfo(vulkanDevice, vulkanVisualizer, vulkanRenderInfo, androidNativeAssetManager);
}

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanDraw(JNIEnv *env, jobject thisObj) {
    if(fistDraw){
        lastDrawTime = std::chrono::high_resolution_clock::now();
        lastFrameDuration = 1.0/60.0;
        fistDraw = false;
    }

    vulkanModelInfo->updateUniformBuffer(static_cast<float>(lastFrameDuration));
    drawFrame();

    std::chrono::milliseconds duration(10000);
    std::this_thread::sleep_for( duration );

    // Стабилизация времени кадра
    std::chrono::high_resolution_clock::duration curFrameDuration = std::chrono::high_resolution_clock::now() - lastDrawTime;
    std::chrono::high_resolution_clock::duration sleepDuration = std::chrono::milliseconds(static_cast<int>(1.0/60.0 * 1000.0)) - curFrameDuration;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count() > 0) {
        std::this_thread::sleep_for(sleepDuration);
    }
    // Расчет времени кадра
    lastFrameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastDrawTime).count() / 1000.0;
    lastDrawTime = std::chrono::high_resolution_clock::now(); // TODO: Возможно - правильнее было бы перетащить в начало цикла
}

JNICALL
void Java_com_example_devnul_vulkanexample_VulkanDrawThread_vulkanDestroy(JNIEnv *env, jobject thisObj) {
    // Ждем завершения работы Vulkan
    vkQueueWaitIdle(vulkanDevice->vulkanGraphicsQueue);
    vkQueueWaitIdle(vulkanDevice->vulkanPresentQueue);
    vkDeviceWaitIdle(vulkanDevice->vulkanLogicalDevice);

    delete vulkanModelInfo;
    delete vulkanRenderInfo;
    delete vulkanVisualizer;
    delete vulkanDevice;

    vulkanModelInfo = nullptr;
    vulkanRenderInfo = nullptr;
    vulkanVisualizer = nullptr;
    vulkanDevice = nullptr;

    fistDraw = true;
}

}