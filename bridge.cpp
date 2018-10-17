#define ANDROID_SMP 0

#include <android/log.h>
#include <unistd.h>
#include "stdio.h"
#include "stdlib.h"
#include <jni.h>
#include "sys/system_properties.h"
#include <sys/time.h>
#include "universal.h"

static void (*MSHookFunction)(void *symbol, void *replace, void **result);

typedef void* bridge_dlopen(const char* filename, int flag);

void load_payload() {
    void* openfun = 0, *payload = 0, *houdini = 0;
    houdini = dlopen("libhoudini.so", RTLD_NOW);
    LOGD("libhoudini.so handle:%p\n", houdini);
    if (houdini) {
      openfun = dlsym(houdini, "dvm2hdDlopen");
      LOGD("openfun:%p\n", openfun);
      if (!openfun) {
        void** itf = (void**)dlsym(houdini, "NativeBridgeItf");
        if (itf) {
          openfun = itf[2];
          LOGD("itf openfun:%p\n", openfun);
        }
      }
      payload = ((bridge_dlopen*)openfun)("/data/local/tmp/libpayload.so", RTLD_NOW);
      LOGD("libpayload.so handle:%p\n", payload);
    }
}


static bool hook_once = false;
static void* (*fopen_ori)(const char * path, const char * mode);
static void* ($fopen)(const char * path, const char * mode) {
	if (!hook_once && strstr(path, "files/.lineagem/clientdata.json")) {
		hook_once = true;
		LOGD("fopen:%s!", path);
		load_payload();
	}
	return fopen_ori(path, mode);
}


static
int init_shooting() {
	const char* path = "/data/local/tmp/libSubstrate.so";
	void *handle = dlopen(path, RTLD_NOW);
  LOGD("handle:%p", handle);
	void *MSGetImageByName = dlsym(handle, "MSGetImageByName");
	LOGD("MSGetImageByName:%p", MSGetImageByName);
	void *MSFindSymbol = dlsym(handle, "MSFindSymbol");
	LOGD("MSFindSymbol:%p", MSFindSymbol);
	*(intptr_t*)&MSHookFunction = (intptr_t)dlsym(handle, "MSHookFunction");
	LOGD("MSHookFunction:%p", MSHookFunction);
	LOGD("$fopen:%p", $fopen);
	MSHookFunction((void*)fopen, (void*)$fopen, (void**)&fopen_ori);
	LOGD("$fopen_ori:%p", fopen_ori);
	return 0;
}


extern "C"
int init_func(char * str){
  static bool once = false;
  __system_property_set("ttm.token", str);
  LOGD("init_func once:%d token:%s", once, str);
  if (once)
    return 0;
  LOGD("%s, hook in pid = %d\n", str, getpid());
  init_shooting();
  LOGD("init_func ended");
  once = true;
  return 0;
}
