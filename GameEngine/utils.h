#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glad.h>
#include <uv.h>

/* Math functions */

#define PRIME_1 97
#define PRIME_2 71

inline int mod(int a, int b) {
	int r = a % b;
	return r < 0 ? r + b : r;
}

inline int hash(const char* s, const int a, const int m) {
	long hash = 0;
	const int len_s = strlen(s);
	for (int i = 0; i < len_s; i++) {
		hash += (long)pow(a, len_s - (i + 1)) * s[i];
		hash = hash % m;
	}
	return (int)hash;
}

inline int get_hash(const char* s, const int m, const int i) {
	const int hash_a = hash(s, PRIME_1, m);
	const int hash_b = hash(s, PRIME_2, m);
	return (hash_a + (i * (hash_b + 1))) % m;
}

/* String functions */
#define TAB_LEN 4

inline size_t linecount(const char* s) {
	unsigned char* c = s;
	size_t i = (*c != '\0');
	while (*c) {
		if (*c++ == '\n')
			i++;
	}
	return i;
}

inline size_t linelen(const char* s) {
	unsigned char* c = s;
	size_t i = 0;
	while (*c) {
		if (*c == '\n')
			return i;
		else if (*c > 0x1F && *c < 0x80)
			i++;
		c++;
	}
	return i;
}

inline char **tok_to_list(const char* line, const char *delim) {
	char* tmp[1024];
	char dup[1024];
	const char* tok;
	int i;
	strcpy(dup, line);
	for (i = 0, tok = strtok(dup, delim); tok && *tok; i++, tok = strtok(NULL, delim))
		tmp[i] = strdup(tok);
	char **arr = memcpy(malloc(i * sizeof(char*)), tmp, i * sizeof(char*));
	return arr;
}

inline int *tok_to_int(char* line, const char *delim) {
	int tmp[4048], i;
	const char* tok;
	for (i = 0, tok = strtok(line, delim); tok && *tok; i++, tok = strtok(NULL, delim))
		tmp[i] = atoi(tok);
	int *arr = malloc(i * sizeof(int));
	memcpy(arr, tmp, i * sizeof(int));
	return arr;
}

inline void read_file(char* filepath, char* fileContent) {
	FILE *f;
	char c;
	int index = 0;
	f = fopen(filepath, "rt");
	while ((c = fgetc(f)) != EOF) {
		fileContent[index] = c;
		index++;
	}
	fileContent[index] = '\0';
}

/* Colors */
#define MAX_COLOR 0xFF;

typedef enum color {
	BLACK = 0x000000,	//0x80
	DARK_BLUE = 0x0000AA,	//0x81
	DARK_GREEN = 0x00AA00,	//0x82
	DARK_AQUA = 0x00AAAA,	//0x83
	DARK_RED = 0xAA0000,	//0x84
	DARK_PURPLE = 0xAA00AA,	//0x85
	GOLD = 0xFFAA00,	//0x86
	GRAY = 0xAAAAAA,	//0x87
	DARK_GRAY = 0x555555,	//0x88
	BLUE = 0x5555FF,	//0x89
	GREEN = 0x55FF55,	//0x8A
	AQUA = 0x55FFFF,	//0x8B
	RED = 0xFF5555,	//0x8C
	LIGHT_PURPLE = 0xFF55FF,	//0x8D
	YELLOW = 0xFFFF55,	//0x8E
	WHITE = 0xFFFFFF,	//0x8F
} color_t;
extern const unsigned int color_table[];

/* OpenGL helper functions */

inline GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	char* error = "NO_ERROR";
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		switch (errorCode) {
			case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
			case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
			case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		fprintf(stderr, "%s | %s (%i)\n", error, file, line);
	}
	return errorCode;
}
#define glCheckError()	   glCheckError_(__FILE__, __LINE__)

/* Async functions */
inline struct sockaddr_storage* get_address(uv_tcp_t* peer, size_t* addrlen) {
	*addrlen = sizeof(struct sockaddr_storage);
	struct sockaddr_storage* addr = malloc(*addrlen);
	uv_tcp_getpeername(peer, &addr, &addrlen);
	return addr;
}

inline void print_address(struct sockaddr_storage* addr, size_t addrlen) {
	char hostbuf[NI_MAXHOST], portbuf[NI_MAXSERV];
	if (getnameinfo(&addr, addrlen, hostbuf, NI_MAXHOST, portbuf, NI_MAXSERV, 0) == 0)
		printf("client (%s:%s) connected\n", hostbuf, portbuf);
	else
		printf("client (unknown) connected\n");
}

/* Lua helper functions */
#define LUA_ENUM(L, name, val) \
  lua_pushlstring(L, #name, sizeof(#name)-1); \
  lua_pushnumber(L, val); \
  lua_settable(L, -3);
#define C_ENUM_HELPER(cname, luaname)  cname,
#define LUA_ENUM_HELPER(cname, luaname) LUA_ENUM(L, luaname, cname)