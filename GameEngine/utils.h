#pragma once
#include "platform.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <uv.h>
#include <lauxlib.h> 

// Use this magic number to avoid floating point issues
#define LARGE_ELEMENT_FI 1.01239812

#define _LITTLE_ENDIAN_ (((union { unsigned x; unsigned char c; }){1}).c)

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
	return glfwGetTime() * 1000.0;
}

// Random UUID generator
inline void random_uid(UID *id) {
	static int random_init = 0;
	if (!random_init++) 
		srand((unsigned int)get_time()*1000000);
	for (int i = 0; i < 4; i++)
		((uint16_t*)id)[i] = rand();
}

inline void print_uuid(UID id) {
	printf("%llx\n", id);
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

inline uint32_t crc32(const unsigned char *s, unsigned int len)
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
//TODO: faster/better hash function
inline uint32_t hash_int(uint64_t key) {
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

	return (uint32_t)key;
}

inline uint64_t hash_string(const char* keystring) {
	return crc32((unsigned char*)(keystring), strlen(keystring));
}

static const unsigned short MortonTable256[256] = {
  0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015,
  0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
  0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115,
  0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
  0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
  0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
  0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515,
  0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
  0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
  0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
  0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
  0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
  0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
  0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
  0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
  0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
  0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
  0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
  0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
  0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
  0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
  0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
  0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
  0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
  0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
  0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
  0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
  0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
  0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
  0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
  0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
  0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

inline uint32_t mortonencode(short x, short y) {
	return	MortonTable256[y >> 8] << 17 |
			MortonTable256[x >> 8] << 16 |
			MortonTable256[y & 0xFF] << 1 |
			MortonTable256[x & 0xFF];
}

// OpenGL helper functions

inline GLenum gl_check_error_(const char* file, int line) {
	GLenum errorCode;
	char* error = "NO_ERROR";
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		switch (errorCode) {
			case GL_INVALID_ENUM:					error = "INVALID_ENUM"; break;
			case GL_INVALID_VALUE:					error = "INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:				error = "INVALID_OPERATION"; break;
			case GL_OUT_OF_MEMORY:					error = "OUT_OF_MEMORY"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:	error = "INVALID_FRAMEBUFFER_OPERATION"; break;
			default:								error = "UNKOWN GL ERROR"; break;
		}
		fprintf(stderr, "%s | %s (%i)\n", error, file, line);
	}
	return errorCode;
}
#define gl_check_error()	   gl_check_error_(__FILE__, __LINE__)


inline ALenum al_check_error_(const char* file, int line) {
	ALenum errorCode;
	char* error = "NO_ERROR";
	while ((errorCode = alGetError()) != AL_NO_ERROR) {
		switch (errorCode) {
			case AL_INVALID_NAME:					error = "AL_INVALID_NAME"; break;
			case AL_INVALID_ENUM:					error = "AL_INVALID_ENUM"; break;
			case AL_INVALID_VALUE:					error = "AL_INVALID_VALUE"; break;
			case AL_INVALID_OPERATION:				error = "AL_INVALID_OPERATION"; break;
			case AL_OUT_OF_MEMORY:					error = "AL_OUT_OF_MEMORY"; break;
			default:								error = "UNKNOWN AL ERROR"; break;
		}
		fprintf(stderr, "%s | %s (%i)\n", error, file, line);
	}
	return errorCode;
}
#define al_check_error()	   al_check_error_(__FILE__, __LINE__)


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

//Set component at index -1 from table at index
inline void set_component_from_table(lua_State* L, int index) {
	lua_pushvalue(L, index);
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		lua_pushvalue(L, -2);
		lua_pushvalue(L, -2);
		lua_settable(L, -6);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

inline uint64_t hash_luatable(lua_State* L, int index) { //TODO: recursive for tables in tables
	char data[512] = "{";
	int len = 1;
	lua_pushvalue(L, index);
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		lua_pushvalue(L, -2);
		const char *key = lua_tostring(L, -1); //TODO: add more types
		const char *value = lua_tostring(L, -2);
		len += sprintf(&data[len], "%s = %s\n", key, value);
		lua_pop(L, 2);
	}
	lua_pop(L, 1);
	data[len++] = '}';
	return crc32(data, len);
}

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
	
#define PUSH_LUA_POINTER(_L, _metaname, ptr) \
	*(void**)lua_newuserdata(_L, sizeof(void*)) = ptr, luaL_setmetatable(_L, _metaname)

#define POP_LUA_POINTER(_L, _metaname, _idx, _Type) \
	*(_Type**)luaL_checkudata(_L, _idx, _metaname)

// String functions
#define TAB_LEN 4

inline size_t linecount(const char* s) {
	const char* c = s;
	size_t i = (*c != '\0');
	while (*c) {
		if (*c++ == '\n')
			i++;
	}
	return i;
}

inline size_t linelen(const char* s) {
	const char* c = s;
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

inline size_t textcharcount(const char* s) {
	const char* c = s;
	size_t i = 0;
	while (*c) {
		if (*c > 0x1F && *c < 0x80) i++;
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

inline int allign_ptr(void* ptr, size_t allign, size_t extra) {
	int adjustment = allign - ((intptr_t)ptr & (allign - 1));
	int needed = extra;
	if (adjustment < needed) {
		needed -= adjustment;
		adjustment += allign * (needed / allign);
		if (needed % allign > 0)
			adjustment += allign;
	}
	return adjustment;
}

inline int setbitrange(int l, int r) {
	return (((1 << (l - 1)) - 1) ^((1 << (r)) - 1));
}


inline int32_t convert_little_int(char* buffer, size_t len) {
	int32_t a = 0;
	if (_LITTLE_ENDIAN_)
		memcpy(&a, buffer, len);
	else
		for (size_t i = 0; i < len; ++i)
			(char*)(&a)[3 - i] = buffer[i];
	return a;
}


inline size_t load_wav_header(const char* data, size_t len, byte* channels, int* samplerate, byte* bitspersample, ushort* compression) {
	//the RIFF header
	if (strncmp(data, "RIFF", 4) != 0)
		return -1;
	//the WAVE header
	if (strncmp(&data[8], "WAVE", 4) != 0)
		return -1;
	//the fmt
	*compression = convert_little_int(&data[20],2);
	*channels = convert_little_int(&data[22], 2);
	*samplerate = convert_little_int(&data[24], 4);
	*bitspersample = convert_little_int(&data[34], 2);
	//the data header
	if (strncmp(&data[36], "data", 4) != 0)
		return -1;
	//return size of data chunk
	return convert_little_int(&data[40], 4);
}

#define WAVE_HEADER_SIZE 44
inline char* load_wav(const char* data, size_t* len, uint* channels, uint* samplerate, uint* bitspersample, int* compression) {
	size_t size = load_wav_header(data, *len, channels, samplerate, bitspersample, compression);
	if (size < 0)
		return NULL;
	assert(size + WAVE_HEADER_SIZE == *len);
	char* buffer = malloc(size);
	if (buffer)
		memcpy(buffer, data + WAVE_HEADER_SIZE, size);
	*len = size;
	return buffer;
}