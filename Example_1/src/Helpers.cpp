#include "Helpers.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <fstream>

// Читаем побайтово файлик
std::vector<unsigned char> readFile(const std::string& filename) {
    // Открываем файлик в бинарном режиме чтения + чтение с конца
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        printf("Failed to open file %s!", filename.c_str());
        fflush(stdout);
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
