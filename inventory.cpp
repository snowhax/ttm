
#include "inventory.h"
#include "resource.h"

Variant *Inventory::getAll() {
  Variant *pool = new Variant(rapidjson::kArrayType);
  Allocator &al = pool->GetAllocator();

  int mi = r(COM::alsr(MemAddrMobileInventory));
  obj icTable = Resource::getTable(COM::alsr(KeyPtrItemClass));
  obj nTable = Resource::getTable(COM::alsr(KeyPtrDescData));

  typedef struct {
    obj itemData;
    obj emplace;
  } Inventory;

  std::vector<Inventory> *inventories = (std::vector<Inventory> *)(mi + 0x4);

  for (Inventory &inventory : *inventories) {
    obj item = inventory.itemData;
    int itemId = r(item + 0x30);
    int categoryId = r(r(item + 0x90) + 0x28);
    int nameId = Resource::getValue<int, 0x80, 0x1c>(icTable, itemId);
    std::string name = Resource::getName(nTable, nameId);
    uint64_t count = *(uint64_t *)(item + 0x20);

    Value v(rapidjson::kObjectType);
    v.AddMember("ptr", item, al);
    v.AddMember("itemId", itemId, al);
    v.AddMember("categoryId", categoryId, al);
    v.AddMember("nameId", nameId, al);
    v.AddMember("name", name, al);
    v.AddMember("count", (uint64_t)count, al);
    pool->PushBack(v, al);
  }

  return pool;
}

int Inventory::limit() {
  int mi = r(COM::alsr(MemAddrMobileInventory));
  return r(mi + 0x10);
}


bool Inventory::useItemById(int itemId) {
  Variant *all = Inventory::getAll();
  int ptr = 0;
  for (auto &item : all->GetArray()) {
    if (item.GetInt("itemId") == itemId) {
      ptr = item.GetInt("ptr");
      break;
    }
  }
  if (ptr) useItemByPtr(ptr);
  delete all;
  return ptr != 0;
}


void Inventory::useItemByPtr(obj item) {
  static fun_3 use = (fun_3)(COM::alsr(FunAddrUseItem));
  int mi = r(COM::alsr(MemAddrMobileInventory));
  use(mi, item, 0);
}
