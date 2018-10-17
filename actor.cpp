
#include "actor.h"
#include "entity.h"
#include "resource.h"

Variant *Actor::props() {
  Variant *pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();

  int sm = r(COM::alsr(MemAddrCharacterInfo));

  obj idt = r(sm + 0xc);
  std::stack<obj> cache;

  static std::map<int, std::string> propMap = {
    {0,  "Lv"},
    {1,  "Exp"},
    {2,  "ExpPercen"},
    {3,  "MaxHp"},
    {4,  "NowHp"},
    {5,  "MaxMp"},
    {6,  "NowMp"},
    {7,  "Propensity"},
    {9,  "Diamond"},
    {11, "Leaf"},
  };

  while (idt || !cache.empty()) {
    while (idt) {
      int propId = r(idt + 0x10);
      uint64_t value = *(uint64_t*)(idt + 0x30);
      std::string strkey = "unknown" + COM::toString<int>(propId);
      std::map<int, std::string>::iterator iter;
      iter = propMap.find(propId);
      if(iter != propMap.end())
        strkey = iter->second;

      Value key;
      key.SetString(strkey.c_str(), strkey.length(), al);
      pool->AddMember(key, value, al);
      cache.push(idt);
      idt = r(idt + 0x8);
    }
    if (!cache.empty()) {
      idt = cache.top();
      cache.pop();
      idt = r(idt + 0xc);
    }
  }

  int jobId = Actor::jobId();
  pool->AddMember("Job", jobId, al);

  return pool;
}

char Actor::_self(signed char op) {
  int CL = r(COM::alsr(MemAddrClientDataManager));
  int data = r(CL+0xc);
  char ori = *(char*)(data+0x43);
  if (op != -1) *(char*)(data+0x43) = op;
  return ori;
}

char Actor::self(signed char op) {
  int SO = r(COM::alsr(MemAddrSelectedObjectManager));
  char ori = Actor::_self();
  if (op != -1) {
    static fun_2 open = (fun_2)(COM::alsr(FunAddrSetSelf));
    open(SO, op);
  }
  return ori;
}

char Actor::automan(signed char op) {
  int AS = r(COM::alsr(MemAddrAutoSelector));
  char ori = *(char*)(AS+0x8);
  if (op != -1) {
    static fun_2 open = (fun_2)(COM::alsr(FunAddrSetAuto));
    open(AS, op);
  }
  return ori;
}

char Actor::attach(signed char op) {
  int AS = r(COM::alsr(MemAddrCharacterPlayManager));
  char ori = *(char*)(AS+0x18);
  if (op != -1) {

  }
  return ori;
}

int Actor::jobId() {
  int CI = r(COM::alsr(MemAddrCharacterInfo));
  return r(CI + 0x3c);
}


Variant* Actor::stats() {
  Variant *pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();

  int sm = r(COM::alsr(MemAddrStatManager));

  obj idt = r(sm + 0xc);
  std::stack<obj> cache;

  while (idt || !cache.empty()) {
    while (idt) {
      int propId = r(idt + 0x10);
      int value = r(idt + 0x2c);
      std::string strId = COM::toString<int>(propId);
      Value key;
      key.SetString(strId.c_str(), strId.length(), al);
      pool->AddMember(key, value, al);
      cache.push(idt);
      idt = r(idt + 0x8);
    }
    if (!cache.empty()) {
      idt = cache.top();
      cache.pop();
      idt = r(idt + 0xc);
    }
  }

  return pool;
}

Variant* Actor::selected() {
  Variant* entity = NULL;
  int so = r(COM::alsr(MemAddrSelectedObjectManager));
  if (r(so+0x8) != (so+0x8)) {
    obj luid = so + 0x1c;
    entity = Entity::get(luid);
  }
  return entity;
}

int* Actor::pos() {
  static int pos[3] = {0};
  pos[0] = 0, pos[1] = 0, pos[2] = 0;
  obj actor = r(r(COM::alsr(MemAddrActor)));
  if (actor) {
    int map = r(COM::alsr(MemAddrMapSystem));
    pos[0] = r(actor + 0xd0);
    pos[1] = r(actor + 0xd4);
    pos[2] = r(map + 0x4);
  }
  return pos;
}

void Actor::select(obj entity) {
  static void *Gluid = 0;
  if (!Gluid)
    Gluid = malloc(0x24);

  void *luid = (void *)Entity::getLuid(entity);
  memcpy(Gluid, luid, 0x24);

  int args[0x24] = {0};

  int *item = (int *)malloc(sizeof(int) * 4);
  item[0] = (int)&args;
  item[1] = (int)&args;
  item[2] = (int)Gluid;
  item[3] = 0;

  args[0] = (int)item;
  args[1] = (int)item;
  args[2] = 0x1;
  args[3] = 0xd;
  args[4] = 0x2;
  args[5] = 0x10001;

  LOGD("选择 :%p %s", entity, Entity::getName(entity).c_str());

  static fun_1 select = (fun_1)(COM::alsr(FunAddrSelect));
  select((int)&args);
}
