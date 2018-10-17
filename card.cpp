
#include "card.h"
#include "resource.h"

Variant *Card::getAll() {
  Variant *pool = new Variant(rapidjson::kArrayType);
  Allocator &al = pool->GetAllocator();

  int bc = r(COM::alsr(MemAddrBuffCardSystem));
  obj nTable = Resource::getTable(COM::alsr(KeyPtrDescData));
  obj cTable = Resource::getTable(COM::alsr(KeyPtrBuffCard));

  obj begin = r(bc + 0x14);
  obj end = bc + 0xc;

  obj idt = begin;
  while (idt != end) {
    int cardId = r(idt + 0x10);
    int nameId = Resource::getValue<int, 0x48, 0x14>(cTable, cardId);
    std::string name = Resource::getName(nTable, nameId);

    LOGD("card:%d name:%s", cardId, name.c_str());

    Value card(rapidjson::kObjectType);
    card.AddMember("cardId", cardId, al);
    card.AddMember("nameId", nameId, al);
    card.AddMember("name", name, al);
    pool->PushBack(card, al);

    if (r(idt + 0xc)) {
      idt = r(idt + 0xc);
      while (r(idt + 0x8))
        idt = r(idt + 0x8);
      continue;
    }
    obj pre = idt;
    idt = r(idt + 0x4);
    if (r(idt + 0xc) != pre)
      continue;

    do {
      pre = idt;
      idt = r(idt + 0x4);
    } while(r(idt + 0xc) == pre);

    if (idt == r(pre + 0xc))
      idt = pre;
  }
  return pool;
}
