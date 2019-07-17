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