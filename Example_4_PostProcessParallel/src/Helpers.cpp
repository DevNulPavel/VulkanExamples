#include "Helpers.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <fstream>

#ifdef _MSVC_LANG
	#include <Windows.h>
#endif 

// Читаем побайтово файлик
std::vector<unsigned char> readFile(const std::string& filename) {
    // Открываем файлик в бинарном режиме чтения + чтение с конца
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        LOG("Failed to open file %s!", filename.c_str());
        throw std::runtime_error("Failed to open file!");
    }
    
    // Получаем размер файлика
    size_t fileSize = (size_t) file.tellg();
    std::vector<unsigned char> buffer(fileSize);
    
    // Переходим в начало файла и читаем данные
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    
    file.close();
    
    return buffer;
}

std::chrono::high_resolution_clock::time_point timestampBegin(){
    return std::chrono::high_resolution_clock::now();
}

void timestampEndMicroSec(const std::chrono::high_resolution_clock::time_point& time1, const char* infoText){
    std::chrono::high_resolution_clock::duration elapsed = std::chrono::high_resolution_clock::now() - time1;
    int64_t elapsedMicroSec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    LOG("%s: %lldmicroSec\n", infoText, elapsedMicroSec);
}

#ifdef _MSVC_LANG
int __cdecl LOG(const char *format, ...) {
	char str[1024];

	va_list argptr;
	va_start(argptr, format);
	int ret = vsnprintf(str, sizeof(str), format, argptr);
	va_end(argptr);

	OutputDebugStringA(str);

	return ret;
}
#endif
