#define ANDROID_SMP 0

#include "Dalvik.h"
#include "android_runtime/AndroidRuntime.h"
#include "api.h"
#include "lua.hpp"
#include "stdio.h"
#include "stdlib.h"
#include "universal.h"
#include <android/log.h>
#include <jni.h>
#include <sys/time.h>
#include <unistd.h>

static lua_State *GL;

typedef struct {
  const char *clazz;
  const char *method;
  const char *sig;
  DalvikNativeFunc replace;
  void *oriMethod;
} HookCtx;

static void (*MSHookFunction)(void *symbol, void *replace, void **result);

static JNIEnv *GetEnv() {
  int status;
  JNIEnv *envnow = NULL;
  JavaVM *jvm = android::AndroidRuntime::getJavaVM();
  status = jvm->GetEnv((void **)&envnow, JNI_VERSION_1_4);
  if (status < 0)
    return NULL;
  return envnow;
}

static int isClearException(JNIEnv *jenv) {
  jthrowable exception = jenv->ExceptionOccurred();
  if (exception != NULL) {
    jenv->ExceptionDescribe();
    jenv->ExceptionClear();
    return true;
  }
  return false;
}

static void method_handler(const u4 *args, JValue *pResult,
                           const Method *method, Thread *self) {
  HookCtx *ctx = (HookCtx *)method->insns;
  ctx->replace(args, pResult);
  Method *oriMethod = reinterpret_cast<Method *>(ctx->oriMethod);
  oriMethod->nativeFunc(args, pResult, oriMethod, self);
}

static int64_t getCurrentTime() { return time(NULL); }

static int64_t last = 0;
static void tick(const u4 *args, JValue *pResult) {
  int64_t now = getCurrentTime();

  if (now - last > 5) {
    last = now;

    LOGD("enter tick:%lld", now);

    const char *error = NULL;
    size_t size = 0;

    lua_getglobal(GL, "tick");
    if (!lua_isfunction(GL, -1)) {
      error = "where are the tick function? :(";
    } else {
      int status = lua_pcall(GL, 0, 0, 0);
      if (status != LUA_OK) {
        if (status != LUA_YIELD)
          error = lua_tolstring(GL, -1, &size);
        if (status == LUA_YIELD)
          error = "can't sleep(yield) in tick :(";
      }
    }
    if (error)
      LOGD("error:%s!", error);
  }
}

static bool hookJava() {
  JNIEnv *jenv = GetEnv();
  if (!jenv)
    return false;

  HookCtx *ctx = (HookCtx *)malloc(sizeof(HookCtx));
  ctx->clazz = "org/cocos2dx/lib/Cocos2dxRenderer";
  ctx->method = "nativeRender";
  ctx->sig = "()V";
  ctx->replace = tick;

  jclass clazz = jenv->FindClass(ctx->clazz);
  if (isClearException(jenv))
    return false;

  jmethodID methodId = jenv->GetStaticMethodID(clazz, ctx->method, ctx->sig);
  if (isClearException(jenv))
    return false;

  Method *method = (Method *)methodId;
  if (method->nativeFunc == method_handler)
    return false;

  Method *oriMethod = (Method *)malloc(sizeof(Method));
  memcpy(oriMethod, method, sizeof(Method));

  ctx->oriMethod = (void *)oriMethod;
  method->insns = (u2 *)ctx;
  method->nativeFunc = method_handler;

  return true;
}

static int open_lua() {
  GL = luaL_newstate();
  if (!GL)
    return -1;
  LOGD("lua_State(GL):%p!", GL);
  luaL_openlibs(GL);
  regLuaFunc(GL);
  int status = luaL_loadfilex(GL, "/data/local/tmp/main.lua", NULL);
  if (status != LUA_OK)
    return -1;
  lua_setglobal(GL, "tick");
  return 0;
}

static int hook_rander() {
  const char *path = "/data/local/tmp/libSubstrate_arm.so";
  void *handle = dlopen(path, RTLD_NOW);
  void *MSGetImageByName = dlsym(handle, "MSGetImageByName");
  LOGD("MSGetImageByName:%p", MSGetImageByName);
  void *MSFindSymbol = dlsym(handle, "MSFindSymbol");
  LOGD("MSFindSymbol:%p", MSFindSymbol);
  *(intptr_t *)&MSHookFunction = (intptr_t)dlsym(handle, "MSHookFunction");
  LOGD("MSHookFunction:%p", MSHookFunction);
  return 0;
}

static void real_init_func() __attribute__((constructor));

void real_init_func() {
	LOGD("real_init_func called\n");

  LOGD("payload init in arm!\n");

  hook_rander();
  open_lua();

  LOGD("real_init_func ended\n");
}
