#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "universal.h"
#include "common.h"

namespace Entity {

  Variant* all();
  Variant* enemy();
  Variant* get(obj luid);

  int getType(int entity);
  int getLuid(int entity);
  uint64_t getId(int entity);
  int* getPos(int entity);
  std::string getName(int entity);
  std::string getJobName(int entity);
  bool hasClan(int entity);
  int getClanLv(int entity);
  std::string getClanName(int entity);
}

#endif
