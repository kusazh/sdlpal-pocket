#ifndef PAL_PLATFORM_IO_H
#define PAL_PLATFORM_IO_H
#include <stdio.h>
FILE *PAL_fopen(const char *path, const char *mode);
#ifndef PAL_PLATFORM_IMPLEMENTATION
#define fopen PAL_fopen
#endif
#endif
