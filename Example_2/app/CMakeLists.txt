cmake_minimum_required(VERSION 3.4.1)

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/JNICode.cpp
             src/main/cpp/VulkanDevice.h
             src/main/cpp/VulkanDevice.cpp
             src/main/cpp/SupportFunctions.h
             src/main/cpp/SupportFunctions.cpp
             src/main/cpp/VulkanCodeWrapper/vulkan_wrapper.h
             src/main/cpp/VulkanCodeWrapper/vulkan_wrapper.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -DVK_USE_PLATFORM_ANDROID_KHR")

find_library(LOG_LIB log)
find_library(ANDROID_LIB android)

target_link_libraries(native-lib ${LOG_LIB} ${ANDROID_LIB})