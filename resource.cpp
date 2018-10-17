
#include "resource.h"


bool Resource::isReady() {
  static bool isReady = false;
  if (!isReady) {
    do {
      if (!r(COM::alsr(MemAddrStaticData))) break;
      if (!r(COM::alsr(MemAddrUserObject))) break;
      if (!r(COM::alsr(MemAddrMobileInventory))) break;
      if (!r(COM::alsr(MemAddrBuffCardSystem))) break;
      if (!r(COM::alsr(MemAddrBuffManager))) break;
      if (!r(COM::alsr(MemAddrStatManager))) break;
      if (!r(COM::alsr(MemAddrCharacterInfo))) break;
      if (!r(COM::alsr(MemAddrSpellManager))) break;
      if (!r(COM::alsr(MemAddrMapSystem))) break;

      if (!Resource::getTable(COM::alsr(KeyPtrDescData))) break;
      if (!Resource::getTable(COM::alsr(KeyPtrItemClass))) break;
      if (!Resource::getTable(COM::alsr(KeyPtrSpellClass))) break;
      if (!Resource::getTable(COM::alsr(KeyPtrBuffCard))) break;

      isReady = true;
    } while(0);
  }
  return isReady;
}

obj Resource::getTable(obj ident) {
  obj sd = r(COM::alsr(MemAddrStaticData));

  obj target = 0;
  obj ptr = ident;

  obj idt = r(sd + 0xc);
  while (idt) {
    obj _ptr = r(idt + 0x10);
    if (_ptr == ptr) {
      target = idt;
      break;
    }
    idt = (ptr > _ptr) ? r(idt + 0xc) : r(idt + 0x8);
  }
  return target ? r(target + 0x14) : 0;
}


obj Resource::getValuePtr(obj table, obj key, size_t tsize, size_t offset) {
  if (!table) return 0;
  obj begin = r(table + 0x4),end = r(table + 0x8);
  obj found = 0;
  int l = 0, r = (end - begin) / tsize - 1;
  while (l < r) {
    int m = (l + r) / 2;
    obj idt = begin + (m * tsize);
    if (r(idt) == key) {
      found = idt;
      break;
    }
    (r(idt) < key) ? (l = m + 1) : r = m;
  }
  return found ? (found + offset) : 0;
}

std::string Resource::getName(obj table, obj key) {
  std::string* name = Resource::getValue<std::string*, 0x1c, 0xc>(table, key);
  return name ? *name : "?";
}
