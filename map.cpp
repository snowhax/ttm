
#include "map.h"
#include "resource.h"

Variant* Map::describe() {
  Variant *pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();

  static fun_1 GetArea = (fun_1)(COM::alsr(FunAddrGetMapArea));
  static fun_1 GetWorld = (fun_1)(COM::alsr(FunAddrGetMapWorld));
  static fun_0 IsSafe = (fun_0)(COM::alsr(FunAddrMapIsSafe));
  static fun_0 IsFight = (fun_0)(COM::alsr(FunAddrMapIsFight));

  int map = r(COM::alsr(MemAddrMapSystem));
  obj nTable = Resource::getTable(COM::alsr(KeyPtrDescData));

  int worldId = -1;
  std::string worldName = "";
  int areaId = -1;
  std::string areaName = "";
  int tag = TagCommon;
  int terrainId = 0;

  obj world = GetWorld(map);
  if (world) {
    worldId = r(world+0x24); //0x1c
    worldName = Resource::getName(nTable, r(world+0x18)); //0x10
  }

  obj area = GetArea(map);
  if (area) {
    areaId = r(area+0x14); //0xc
    areaName = Resource::getName(nTable, r(area+0x20)); //0x18
  }

  if (IsSafe())
    tag = TagSafe;
  else if(IsFight())
    tag = TagFight;

  pool->AddMember("worldId", worldId, al);
  pool->AddMember("worldName", worldName, al);
  pool->AddMember("areaId", areaId, al);
  pool->AddMember("areaName", areaName, al);
  pool->AddMember("tag", tag, al);

  return pool;
}
