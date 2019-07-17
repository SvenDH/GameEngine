#pragma once
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <uv.h>
#include <lauxlib.h>

// Logging
#ifdef DEBUG
#define log_info(M, ...) fprintf(stdout, "[INFO] (%s:%d)\n" M "\n",\
							__FILE__, __LINE__, ##__VA_ARGS__); \
							fflush(stdout)
#else		
#define log_info(M, ...)	
#endif

// Async functions
inline double get_time() {
	return glfwGetTime();
}

// Random UUID generator
typedef uint16_t UID[8]; //128 bit number

inline void random_uuid(UID id) {
	static int random_init = 0;
	int i;
	if (!random_init++) 
		srand(get_time()*1000000);
	for (i = 0; i < 8; i++)
		id[i] = rand();
}

inline void print_uuid(UID id) {
	for (int i = 0; i < 8; i++)
		printf("%04x ", id[i]);
	printf("\n");
}

// Hashing utils
static unsigned long crc32_tab[] = {
	  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	  0x2d02ef8dL
};

inline unsigned long crc32(const unsigned char *s, unsigned int len)
{
	unsigned int i;
	unsigned long crc32val;

	crc32val = 0;
	for (i = 0; i < len; i++)
	{
		crc32val =
			crc32_tab[(crc32val ^ s[i]) & 0xff] ^
			(crc32val >> 8);
	}
	return crc32val;
}

inline unsigned int hash_int(const char* keystring, int len, int table_size) {

	unsigned long key = crc32((unsigned char*)(keystring), len);

	/* Robert Jenkins' 32 bit Mix Function */
	key += (key << 12);
	key ^= (key >> 22);
	key += (key << 4);
	key ^= (key >> 9);
	key += (key << 10);
	key ^= (key >> 2);
	key += (key << 7);
	key ^= (key >> 12);

	/* Knuth's Multiplicative Method */
	key = (key >> 3) * 2654435761;

	return key % table_size;
}

// Colors
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

// OpenGL helper functions

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
#define gl_check_error()	   glCheckError_(__FILE__, __LINE__)

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

// Lua helper functions
inline int is_in_table(lua_State *L, const char* str, int index) {
	lua_pushvalue(L, index);
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		lua_pushvalue(L, -2);
		const char *key = lua_tostring(L, -1);
		const char *value = lua_tostring(L, -2);
		if (strcmp(value, str) == 0) {
			lua_pop(L, 3);
			return 1;
		}
		lua_pop(L, 2);
	}
	lua_pop(L, 1);
	return 0;
}

inline void print_table(lua_State *L, int index) {
	lua_pushvalue(L, index);
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		lua_pushvalue(L, -2);
		const char *key = lua_tostring(L, -1);
		const char *value = lua_tostring(L, -2);
		printf("%s => %s\n", key, value);
		lua_pop(L, 2);
	}
	lua_pop(L, 1);
}

#define LUA_ENUM(L, name, val) \
	lua_pushlstring(L, #name, sizeof(#name)-1); \
	lua_pushnumber(L, val); \
	lua_settable(L, -3);
#define C_ENUM_HELPER(cname, luaname)  cname,
#define LUA_ENUM_HELPER(cname, luaname) LUA_ENUM(L, luaname, cname)

#define create_lua_lib(L, metaname, lib) \
	luaL_newlib(L, lib); \
	lua_setglobal(L, metaname)

#define create_lua_type(L, metaname, methods) \
	luaL_newmetatable(L, metaname); \
	lua_pushvalue(L, -1); \
	lua_setfield(L, -2, "__index"); \
	luaL_setfuncs(L, methods, 0); \
	lua_pop(L, 1)

#define create_lua_class(L, metaname, init, methods) \
	luaL_newmetatable(L, metaname); \
	lua_pushvalue(L, -1); \
	lua_setfield(L, -2, "__index"); \
	luaL_setfuncs(L, methods, 0); \
	lua_newtable(L); \
	lua_pushcfunction(L, init); \
	lua_setfield(L, -2, "__call"); \
	lua_setmetatable(L, -2); \
	lua_setglobal(L, metaname)
	
#define PUSH_LUA_POINTER(L, metaname, ptr) \
	*(void**)lua_newuserdata(L, sizeof(void*)) = ptr, luaL_setmetatable(L, metaname)

// String functions
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

inline int str_to_mode(const char* mode) {
	int flags, m, o;
	switch (*mode++) {
	case 'r':	/* open for reading */
		m = O_RDONLY;
		o = 0;
		break;
	case 'w':	/* open for writing */
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;
	case 'a':	/* open for appending */
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;
	default:	/* illegal mode */
		errno = EINVAL;
		return (0);
	}
	/* [rwa]\+ or [rwa]b\+ means read and write */
	if (*mode == '+' || (*mode == 'b' && mode[1] == '+')) {
		m = O_RDWR;
	}
	flags = m | o;
	return flags;
}

/*
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

// Data struct macros
#define enqueue(b, r, t) \
	if (((b)->tail_ + 1) % (b)->max_ != (b)->head_) { \
		(b)->tail_ = ((b)->tail_ + 1) % (b)->max_; \
		((t*)(b)->data_)[(b)->tail_] = (r); \
		return 0; \
	} \
	else return 1

#define peek(b, r, t) \
	if ((b)->head_ != (b)->tail_) { \
		r = ((t*)(b)->data_)[(b)->head_]; \
	}

#define dequeue(b, r, t) \
	if ((b)->head_ != (b)->tail_) { \
		(b)->head_ = ((b)->head_ + 1) % (b)->max_; \
		r = ((t*)(b)->data_)[(b)->head_]; \
	}

#define push(b, r, t) \
	((t*)(b)->data_)[++(b)->top_] = r

#define pop(b, r, t) \
	r = ((t*)(b)->data_)[(b)->top_--]

*/