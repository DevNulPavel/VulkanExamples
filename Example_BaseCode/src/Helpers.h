#ifndef HELPERS_H
#define HELPERS_H

#include <vector>
#include <string>
#include <chrono>


// Читаем побайтово файлик
std::vector<unsigned char> readFile(const std::string& filename);

std::chrono::high_resolution_clock::time_point timestampBegin();

void timestampEndMicroSec(const std::chrono::high_resolution_clock::time_point& time1, const char* infoText);

#define TIME_BEGIN(NAME) std::chrono::high_resolution_clock::time_point NAME = timestampBegin();
#define TIME_END_MICROSEC(NAME, INFO) timestampEndMicroSec(NAME, INFO)

#define TIME_BEGIN_OFF(NAME) {}
#define TIME_END_MICROSEC_OFF(NAME, INFO) {}

void sleepShort(float milliseconds);

#ifdef _MSC_BUILD
	int __cdecl LOG(const char *format, ...);
#else
    #define LOG(...) {printf(__VA_ARGS__); fflush(stdout);}
#endif

#endif
