#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "universal.h"
#include "common.h"


namespace Resource {
  bool isReady();
  obj getTable(int ident);
  std::string getName(obj table, obj key);
  obj getValuePtr(obj table, obj key, size_t tsize, size_t offset);

  template <typename T, size_t tsize, size_t offset>
  T getValue(obj table, obj key) {
    obj addr = getValuePtr(table, key, tsize, offset);
    return addr ? *reinterpret_cast<T*>(addr) : 0;
  }
}

#endif
