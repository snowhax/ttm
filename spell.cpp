
#include "resource.h"
#include "spell.h"
#include "actor.h"


static int isLearned(int spellId) {
  int SM = r(COM::alsr(MemAddrSpellManager));
  obj idt = r(SM + 0x34);
  obj src = idt;
  while(idt) {
    int Id = r(idt + 0x10);
    if (Id == spellId) return 1;
    (Id < spellId) ? idt = r(idt+0xc) : idt = r(idt+0x8);
  }
  return 0;
}

Variant *Spell::getAll() {
  Variant *pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();

  obj scTable = Resource::getTable(COM::alsr(KeyPtrSpellClass));
  obj nTable = Resource::getTable(COM::alsr(KeyPtrDescData));
  obj limlit = Resource::getTable(COM::alsr(KeyPtrSpellLearnLimit));

  obj roleJob = Actor::jobId();

  std::vector<int[0x8]>* all = (std::vector<int[0x8]> *)(limlit + 0x4);
  for (auto &spell : *all) {
    int spellId = spell[4];
    obj spellClass = Resource::getValuePtr(scTable, spellId, 0xa4, 0x4);
    if (!spellClass)
      continue;

    int jobId = spell[3];
    if (roleJob != jobId)
      continue;

    int nameId = r(spellClass + 0x10);
    int common = r(spellClass + 0x18);
    int level = r(spellClass + 0x1c);
    int spellType = r(spellClass + 0x24);
    int materialId = r(spellClass + 0x34);
    int materialCount = r(spellClass + 0x38);

    std::string name = Resource::getName(nTable, nameId);

    int useHp = r(spellClass + 0x3c);
    int useMp = r(spellClass + 0x40);
    int learned = isLearned(spellId);

    Value key;
    Value v(rapidjson::kObjectType);
    v.AddMember("spellId", spellId, al);
    v.AddMember("common", common, al);
    v.AddMember("nameId", nameId, al);
    v.AddMember("name", name, al);
    v.AddMember("level", level, al);
    v.AddMember("spellType", spellType, al);
    v.AddMember("materialId", materialId, al);
    v.AddMember("materialCount", materialCount, al);
    v.AddMember("useHp", useHp, al);
    v.AddMember("useMp", useMp, al);
    v.AddMember("learned", learned, al);
    std::string str = COM::toString<int>(spellId);
    key.SetString(str.c_str(), str.length(), al);
    pool->AddMember(key, v, al);
  }

  return pool;
}


/*
Variant *Spell::getAll() {
  Variant *pool = new Variant(rapidjson::kArrayType);
  Allocator &al = pool->GetAllocator();

  int sm = r(COM::alsr(MemAddrSpellManager));
  obj scTable = Resource::getTable(COM::alsr(KeyPtrSpellClass));
  obj nTable = Resource::getTable(COM::alsr(KeyPtrDescData));

  obj idt = r(sm + 0x34);
  std::stack<obj> cache;

  while (idt || !cache.empty()) {
    while (idt) {
      int spellId = r(idt + 0x10);
      obj spellClass = Resource::getValuePtr(scTable, spellId, 0xa4, 0x4);
      int nameId = spellClass ? r(spellClass + 0x10) : 0;
      int spellType = spellClass ? r(spellClass + 0x24) : 0;
      int materialId = spellClass ? r(spellClass + 0x34) : 0;
      std::string name = Resource::getName(nTable, nameId);

      int useHp = spellClass ? r(spellClass + 0x3c) : 0;
      int useMp = spellClass ? r(spellClass + 0x40) : 0;

      Value spell(rapidjson::kObjectType);
      spell.AddMember("spellId", spellId, al);
      spell.AddMember("nameId", nameId, al);
      spell.AddMember("name", name, al);
      spell.AddMember("spellType", spellType, al);
      spell.AddMember("materialId", materialId, al);
      spell.AddMember("useHp", useHp, al);
      spell.AddMember("useMp", useMp, al);
      pool->PushBack(spell, al);

      cache.push(idt);
      idt = r(idt+0x8);
    }
    if (!cache.empty()) {
      idt = cache.top();
      cache.pop();
      idt = r(idt + 0xc);
    }
  }

  return pool;
}
*/


void Spell::use(int spellId, signed char isSelf) {
  static fun_2 use = (fun_2)(COM::alsr(FunAddrUseSpell));
  int sm = r(COM::alsr(MemAddrSpellManager));
  if (isSelf == -1) {
    use(sm, spellId);
  } else {
    char ori = Actor::_self();
    LOGD("修改self:%d ori:%d", isSelf, ori);
    Actor::_self(isSelf);
    use(sm, spellId);
    Actor::_self(ori);
  }
}
