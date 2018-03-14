#ifndef COMMON_INCLUDES_H
#define COMMON_INCLUDES_H

// Windows
#ifdef _MSVC_LANG
	#define NOMINMAX 
	#include <windows.h>
#endif // MSV

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
#include <fstream>
#include <algorithm>
#include <limits>


// GLFW include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#endif
