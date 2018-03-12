#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __APPLE__
    #include <OpenCL/opencl.h>
#else
    #include <CL/cl.h>
#endif


// https://habrahabr.ru/post/261323/
// https://habrahabr.ru/post/124925/
// https://ru.wikipedia.org/wiki/OpenCL
// https://developer.apple.com/library/content/documentation/Performance/Conceptual/OpenCL_MacProgGuide/Introduction/Introduction.html

////////////////////////////////////////////////////////////////////////////////

#define STRINGIFY(_STR_) (#_STR_)

int main(int argc, char** argv) {
    

    return 0;
}

