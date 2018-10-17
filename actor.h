#ifndef __ACTOR_H__
#define __ACTOR_H__

#include "universal.h"
#include "common.h"


namespace Actor {
  Variant* props();
  Variant* stats();
  int jobId();
  int* pos();
  char _self(signed char op=-1);
  char self(signed char op=-1);
  char automan(signed char op=-1);
  char attach(signed char op=-1);
  void select(obj entity);
  Variant* selected();
}

#endif
