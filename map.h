#ifndef __MAP_H__
#define __MAP_H__

#include "universal.h"
#include "common.h"

enum MapTag {
  TagCommon,
  TagSafe,
  TagFight,
};

namespace Map {
  Variant* describe();
}

#endif
