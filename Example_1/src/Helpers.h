#ifndef HELPERS_H
#define HELPERS_H

#include <vector>
#include <string>


// Читаем побайтово файлик
std::vector<unsigned char> readFile(const std::string& filename);

#ifdef _MSVC_LANG
	int __cdecl LOG(const char *format, ...);
#else
	#define LOG printf
#endif

#endif
