#pragma once
#define DEBUG

#ifdef _WIN32
#include <malloc.h>
#define aligned_alloc(a, s) _aligned_malloc(s, a)
#endif

#ifdef _WIN32
#define pipe_name "\\\\.\\pipe\\AppConsole.sock"
#else
#define pipe_name "AppConsole.sock"
#endif

#define MAX_PATH_LENGTH 256
#define PAGE_SIZE 4096
#define CHUNK_SIZE PAGE_SIZE
#define MAX_COMPS 32 //(sizeof(int) * 8)