#include "Helpers.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <fstream>

#ifdef _MSC_BUILD
	#include <Windows.h>
#else
    //#include <thread>
    #include <errno.h>
    #include <time.h>
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


#ifdef _MSC_BUILD
	static NTSTATUS(__stdcall *NtDelayExecution)(BOOL Alertable, PLARGE_INTEGER DelayInterval) = (NTSTATUS(__stdcall*)(BOOL, PLARGE_INTEGER)) GetProcAddress(GetModuleHandle("ntdll.dll"), "NtDelayExecution");
	static NTSTATUS(__stdcall *ZwSetTimerResolution)(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution) = (NTSTATUS(__stdcall*)(ULONG, BOOLEAN, PULONG)) GetProcAddress(GetModuleHandle("ntdll.dll"), "ZwSetTimerResolution");
	void sleepShort(float milliseconds) {
		static bool once = true;
		if (once) {
			ULONG actualResolution;
			ZwSetTimerResolution(1, true, &actualResolution);
			once = false;
		}

		LARGE_INTEGER interval;
		interval.QuadPart = -1 * (int)(milliseconds * 10000.0f);
		NtDelayExecution(false, &interval);
	}
#else
	void sleepShort(float milliseconds){
//        long usec = (long)(milliseconds * 1000) * 1000;
//
//        struct timeval tv;
//        fd_set dummy;
//        int s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
//        FD_ZERO(&dummy);
//        FD_SET(s, &dummy);
//        tv.tv_sec = usec / 1000000L;
//        tv.tv_usec = usec % 1000000L;
//        select(0, 0, 0, &dummy, &tv);
        //std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(milliseconds * 1000.0f)));
        
        struct timespec tv;
        /* Construct the timespec from the number of whole seconds... */
        tv.tv_sec = (time_t)(milliseconds/1000.0);
        /* ... and the remainder in nanoseconds. */
        tv.tv_nsec = (long) ((milliseconds/1000.0 - tv.tv_sec) * 1e+9);
        
        while (1){
            /* Sleep for the time specified in tv. If interrupted by a
             signal, place the remaining time left to sleep back into tv. */
            int rval = nanosleep (&tv, &tv);
            if (rval == 0)
            /* Completed the entire sleep time; all done. */
                return;
            else if (errno == EINTR)
            /* Interrupted by a signal. Try again. */
                continue;
            else
            /* Some other error; bail out. */
                return;
        }
	}
#endif
