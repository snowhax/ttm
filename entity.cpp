
#include "entity.h"
#include <typeinfo>

uint64_t Entity::getId(int entity) {
  int luid = r(entity + 0xa4);
  return *(uint64_t*)(luid + 0xc);
}

/*
int Entity::getId2(int entity) {
  int luid = r(entity + 0xa4);
  return r(luid + 0x14)
}
*/

int Entity::getLuid(int entity) {
  return r(entity + 0xa4);
}

int Entity::getType(int entity) {
  int luid = r(entity + 0xa4);
  return r(luid + 0x18);
}

int *Entity::getPos(int entity) {
  return *(int **)(entity + 0x13c);
}

std::string Entity::getName(int entity) {
  return *(std::string *)(entity + 0x8);
}

std::string Entity::getJobName(int entity) {
  return *(std::string *)(entity + 0x14);
}

bool Entity::hasClan(int entity) {
  int clan = r(entity + 0x4c);
  return clan != 0;
}

int Entity::getClanLv(int entity) {
  int clan = r(entity + 0x4c);
  return r(clan + 0x18);
}

std::string Entity::getClanName(int entity) {
  int clan = r(entity + 0x4c);
  return *(std::string *)r(clan + 0x14);
}

static void addEntity(Variant * pool, obj entity) {
  Allocator &al = pool->GetAllocator();

  std::string name = Entity::getName(entity);
  std::string jobName = Entity::getJobName(entity);
  int type = Entity::getType(entity);
  uint64_t id = Entity::getId(entity);
  int *ppos = Entity::getPos(entity);
  int x = ppos ? ppos[0] : 0;
  int y = ppos ? ppos[1] : 0;

  Value item(rapidjson::kObjectType);
  item.AddMember("ptr", entity, al);
  item.AddMember("name", name, al);
  item.AddMember("jobName", jobName, al);
  item.AddMember("type", type, al);
  item.AddMember("id", id, al);

  Value pos(rapidjson::kObjectType);
  pos.AddMember("x", x, al);
  pos.AddMember("y", y, al);
  item.AddMember("pos", pos, al);

  if (Entity::hasClan(entity)) {
    int clanLv = Entity::getClanLv(entity);
    if (clanLv) {
      Value clan(rapidjson::kObjectType);
      std::string clanName = Entity::getClanName(entity);
      clan.AddMember("name", clanName, al);
      clan.AddMember("lv", clanLv, al);
      item.AddMember("clan", clan, al);
    }
  }

  pool->PushBack(item, al);
}

Variant *Entity::all() {
  Variant *pool = new Variant(rapidjson::kArrayType);
  Allocator &al = pool->GetAllocator();

  int um = r(COM::alsr(MemAddrUserObject));

  std::vector<int> *entities = (std::vector<int> *)(um + 0x4);
  for (int entity : *entities) {
    addEntity(pool, entity);
  }

  return pool;
}


Variant* Entity::get(obj luid) {
  Variant *pool = NULL;
  int um = r(COM::alsr(MemAddrUserObject));
  uint64_t tid = *(uint64_t*)(luid + 0xc);
  std::vector<int> *entities = (std::vector<int> *)(um + 0x4);
  for (int entity : *entities) {
    uint64_t id = Entity::getId(entity);
    if (id == tid) {
      pool = new Variant(rapidjson::kObjectType);
      Allocator &al = pool->GetAllocator();

      std::string name = Entity::getName(entity);
      std::string jobName = Entity::getJobName(entity);
      int type = Entity::getType(entity);
      uint64_t id = Entity::getId(entity);
      int *ppos = Entity::getPos(entity);
      int x = ppos ? ppos[0] : 0;
      int y = ppos ? ppos[1] : 0;

      pool->AddMember("ptr", entity, al);
      pool->AddMember("name", name, al);
      pool->AddMember("jobName", jobName, al);
      pool->AddMember("type", type, al);
      pool->AddMember("id", id, al);

      Value pos(rapidjson::kObjectType);
      pos.AddMember("x", x, al);
      pos.AddMember("y", y, al);
      pool->AddMember("pos", pos, al);

      if (Entity::hasClan(entity)) {
        int clanLv = Entity::getClanLv(entity);
        if (clanLv) {
          Value clan(rapidjson::kObjectType);
          std::string clanName = Entity::getClanName(entity);
          clan.AddMember("name", clanName, al);
          clan.AddMember("lv", clanLv, al);
          pool->AddMember("clan", clan, al);
        }
      }

      break;
    }
  }
  return pool;
}

static obj getPlayerInGameData(int type) {
  int PI = r(COM::alsr(MemAddrPlayerInGameData));
  obj idt = r(PI + 0xc);
  obj target = 0;
  while (idt) {
    obj _type = r(idt + 0x10);
    if (_type == type) {
      target = idt;
      break;
    }
    idt = (type > _type) ? r(idt + 0xc) : r(idt + 0x8);
  }
  return target ? target + 0x14 : 0;
}

Variant* Entity::enemy() {
  Variant *pool = new Variant(rapidjson::kArrayType);
  Allocator &al = pool->GetAllocator();

  obj data = getPlayerInGameData(2);
  LOGD("data:%p", data);
  if (data) {
    int UM = r(COM::alsr(MemAddrUserObject));
    std::vector<int> *entities = (std::vector<int> *)(UM + 0x4);

    int count = r(data + 0x8);
    obj idt = r(data + 0x4);
    LOGD("idt begin:%p", idt);
    while (idt != data) {
      LOGD("idt:%p", idt);
      obj luid = r(r(idt + 0x8));
      if (luid) {
        uint64_t tid = *(uint64_t*)(luid + 0xc);
        LOGD("查找tid:%lld", tid);
        for (int entity : *entities) {
          uint64_t id = Entity::getId(entity);
          LOGD("%lld:%lld", tid, id);
          if (id == tid) {
            LOGD("找到luid:%p", entity);
            addEntity(pool, entity);
          }
        }
      }
      idt = r(idt + 0x4);
    }
  }

  return pool;
}
