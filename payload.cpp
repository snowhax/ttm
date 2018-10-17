#define ANDROID_SMP 0

#include "stdio.h"
#include "stdlib.h"
#include "universal.h"
#include <android/log.h>
#include <sys/time.h>
#include <unistd.h>
#include "common.h"
#include "engine.h"


static __attribute__((constructor)) void real_init_func() {
	LOGD("payload init begin!\n");

  Engine::init();

  LOGD("payload init end\n");
}
