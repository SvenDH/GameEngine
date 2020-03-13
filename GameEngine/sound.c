#include "resource.h"
#include "audio.h"
#include "utils.h"

void sound_load(sound_t* sound, const char* data, size_t len) {
	int compression = 0;
	sound->length = len;
	sound->data = load_wav(data, &sound->length, &sound->channels, &sound->rate, &sound->bits, &compression);	
	assert(compression == 1);
}

void sound_unload(sound_t* sound) {
	free(sound->data);
}

//Sound(path)
int w_sound_new(lua_State* L) {
	lua_pushinteger(L, RES_SOUND);
	return w_resource_new(L);
}

//Sound.__load(data)
int w_sound_load(lua_State* L) {
	sound_t* source = (sound_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	size_t len;
	char* data = luaL_checklstring(L, 2, &len);
	sound_load(source, data, len);
	return 0;
}

//Sound.unload()
int w_sound_unload(lua_State* L) {
	sound_t* sound = (sound_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	sound_unload(sound);
	return 0;
}