#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "universal.h"
#include "common.h"


typedef int (*eventHandler)(Value* parms);

typedef struct {
  uint32_t      id;
  std::string*  token;
  int           guard_tick;
  uint64_t      guard_last;
  int           main_tick;
  uint64_t      main_last;
  int           sync_tick;
  uint64_t      sync_last;
  int           is_develop;
  int           is_paused;
  std::map<std::string, eventHandler>* handler;
} Config;


namespace Engine {
  int init();
  void tick();
  Config* config();
  int paused(int op=-1);
  void login();
}

#endif
