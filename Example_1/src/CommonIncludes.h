#ifndef COMMON_INCLUDES_H
#define COMMON_INCLUDES_H


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
//#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
    #include <X11/Xutil.h>
#endif

#ifdef _WIN32
    #pragma comment(linker, "/subsystem:windows")
    #define APP_NAME_STR_LEN 80
#endif

#if defined(VK_USE_PLATFORM_MIR_KHR)
    #warning "Cube does not have code for Mir at this time"
#endif

#ifdef ANDROID
    #include "vulkan_wrapper.h"
#else
    #include <vulkan/vulkan.h>
#endif


#endif
