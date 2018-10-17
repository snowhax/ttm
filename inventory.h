#ifndef __INVENTORY_H__
#define __INVENTORY_H__

#include "universal.h"
#include "common.h"


namespace Inventory {
  Variant* getAll();
  int limit();
  void useItemByPtr(obj item);
  bool useItemById(int classId);
}

#endif
