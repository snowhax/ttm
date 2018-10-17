
#include "buff.h"
#include "resource.h"

Variant *Buff::getAll() {
  Variant *pool = new Variant(rapidjson::kArrayType);
  Allocator &al = pool->GetAllocator();

  int bm = r(COM::alsr(MemAddrBuffManager));
  obj nTable = Resource::getTable(COM::alsr(KeyPtrDescData));

  uint64_t bootTick = COM::getBootTick();

  obj begin = r(bm + 0xc);
  obj end = r(bm + 0x10);
  int n = (end - begin) / 0x70;

  obj idt = begin;
  while (n--) {
    int buffId = r(idt + 0x10);
    int nameId = r(idt + 0x14);
    std::string name = Resource::getName(nTable, nameId);
    int keep = r(idt + 0x54);
    if (keep > 0x278CFF)
      keep = -1;

    uint64_t nowTick = *(uint64_t *)(idt + 0x68);
    int surplus = (nowTick - bootTick) / (1000000*1000);
    if (keep < 0) surplus = -1;

    Value buff(rapidjson::kObjectType);
    buff.AddMember("buffId", buffId, al);
    buff.AddMember("nameId", nameId, al);
    buff.AddMember("name", name, al);
    buff.AddMember("keep", keep, al);
    buff.AddMember("surplus", surplus, al);
    pool->PushBack(buff, al);

    idt += 0x70;
  }

  return pool;
}
