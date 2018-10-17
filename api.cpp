
#include "actor.h"
#include "api.h"
#include "buff.h"
#include "card.h"
#include "entity.h"
#include "inventory.h"
#include "net.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "resource.h"
#include "spell.h"
#include "ui.h"
#include "chat.h"
#include "map.h"
#include <iconv.h>
#include <typeinfo>

int code_convert(const char *from_charset, const char *to_charset,
                 const char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
  iconv_t cd;
  const char **pin = &inbuf;
  char **pout = &outbuf;
  cd = iconv_open(to_charset, from_charset);
  if (cd == 0)
    return -1;
  memset(outbuf, 0, outlen);
  iconv(cd, const_cast<char **>(pin), &inlen, pout, &outlen);
  iconv_close(cd);
  return 0;
}

std::string u2g(std::string &from) {
  if (from.size() == 0 || from.size() > 1024)
    return "";
  size_t outlen = from.size() * 4;
  char *outbuf = (char *)alloca(outlen);
  bzero(outbuf, outlen);
  code_convert("UTF-8", "GBK", from.c_str(), from.size(), outbuf, outlen);
  return std::string(outbuf, outlen);
}

static void show(Variant *var) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  var->Accept(writer);
  LOGD("json:%s", buffer.GetString());
  delete var;
}

static int log(lua_State *L) {
  const char *text = lua_tostring(L, 1);
  LOGD("%s", text);
  return 0;
}

static int net_log(lua_State *L) {
  std::string text = lua_tostring(L, 1);
  Variant* pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();
  pool->AddMember("cmd", "log", al);
  Value parms(rapidjson::kObjectType);
  parms.AddMember("text", text, al);
  pool->AddMember("parms", parms, al);
  Net::send(pool);
  delete pool;
  return 0;
}

static int InventoryAll(lua_State *L) {
  Variant *all = Inventory::getAll();
  lua_newtable(L);
  int i = 0;
  for (auto &item : all->GetArray()) {
    i++;
    lua_newtable(L);

    int ptr = item.GetInt("ptr");
    lua_pushinteger(L, ptr);
    lua_setfield(L, -2, "ptr");

    int itemId = item.GetInt("itemId");
    lua_pushinteger(L, itemId);
    lua_setfield(L, -2, "itemId");

    int categoryId = item.GetInt("categoryId");
    lua_pushinteger(L, categoryId);
    lua_setfield(L, -2, "categoryId");

    int nameId = item.GetInt("nameId");
    lua_pushinteger(L, nameId);
    lua_setfield(L, -2, "nameId");

    std::string name = item.GetString("name");
    lua_pushstring(L, name.c_str());
    lua_setfield(L, -2, "name");

    uint64_t count = item.GetUint64("count");
    lua_pushinteger(L, count);
    lua_setfield(L, -2, "count");

    lua_rawseti(L, -2, i);
  }
  delete all;
  return 1;
}


static int InventoryLimit(lua_State *L) {
  int limit = Inventory::limit();
  lua_pushinteger(L, limit);
  return 1;
}


static int InventoryUseByPtr(lua_State *L) {
  obj ptr = luaL_checkinteger(L, 1);
  Inventory::useItemByPtr(ptr);
  return 0;
}

static int InventoryUseById(lua_State *L) {
  int itemId = luaL_checkinteger(L, 1);
  bool used = Inventory::useItemById(itemId);
  lua_pushboolean(L, used);
  return 1;
}


static int BuffAll(lua_State *L) {
  Variant *all = Buff::getAll();
  lua_newtable(L);
  int i = 0;
  for (auto &item : all->GetArray()) {
    i++;
    lua_newtable(L);

    int buffId = item.GetInt("buffId");
    lua_pushinteger(L, buffId);
    lua_setfield(L, -2, "buffId");

    int nameId = item.GetInt("nameId");
    lua_pushinteger(L, nameId);
    lua_setfield(L, -2, "nameId");

    std::string name = item.GetString("name");
    lua_pushstring(L, name.c_str());
    lua_setfield(L, -2, "name");

    int keep = item.GetInt("keep");
    lua_pushinteger(L, keep);
    lua_setfield(L, -2, "keep");

    int surplus = item.GetInt("surplus");
    lua_pushinteger(L, surplus);
    lua_setfield(L, -2, "surplus");

    lua_rawseti(L, -2, i);
  }
  delete all;
  return 1;
}

static int CardAll(lua_State *L) {
  Variant *all = Card::getAll();
  lua_newtable(L);
  int i = 0;
  for (auto &item : all->GetArray()) {
    i++;
    lua_newtable(L);

    int cardId = item.GetInt("cardId");
    lua_pushinteger(L, cardId);
    lua_setfield(L, -2, "cardId");

    int nameId = item.GetInt("nameId");
    lua_pushinteger(L, nameId);
    lua_setfield(L, -2, "nameId");

    std::string name = item.GetString("name");
    lua_pushstring(L, name.c_str());
    lua_setfield(L, -2, "name");

    lua_rawseti(L, -2, i);
  }
  delete all;
  return 1;
}

static void entity_warp(lua_State *L, Value* item) {
  lua_newtable(L);

  int ptr = item->GetInt("ptr");
  lua_pushinteger(L, ptr);
  lua_setfield(L, -2, "ptr");

  std::string name = item->GetString("name");
  lua_pushstring(L, name.c_str());
  lua_setfield(L, -2, "name");

  std::string jobName = item->GetString("jobName");
  lua_pushstring(L, jobName.c_str());
  lua_setfield(L, -2, "jobName");

  int type = item->GetInt("type");
  lua_pushinteger(L, type);
  lua_setfield(L, -2, "type");

  uint64_t id = item->GetUint64("id");
  lua_pushinteger(L, id);
  lua_setfield(L, -2, "id");

  lua_newtable(L);
  Value pos = item->GetObject("pos");
  int x = pos.GetInt("x");
  int y = pos.GetInt("y");
  lua_pushinteger(L, x);
  lua_setfield(L, -2, "x");
  lua_pushinteger(L, y);
  lua_setfield(L, -2, "y");
  lua_setfield(L, -2, "pos");

  if (item->HasMember("clan")) {
    lua_newtable(L);
    Value clan = item->GetObject("clan");
    int lv = clan.GetInt("lv");
    lua_pushinteger(L, lv);
    lua_setfield(L, -2, "lv");
    std::string name = clan.GetString("name");
    lua_pushstring(L, name.c_str());
    lua_setfield(L, -2, "name");
    lua_setfield(L, -2, "clan");
  }
}

static int EntityAll(lua_State *L) {
  Variant *all = Entity::all();
  lua_newtable(L);
  int i = 0;
  for (auto &item : all->GetArray()) {
    i++;
    entity_warp(L, &item);
    lua_rawseti(L, -2, i);
  }
  delete all;
  return 1;
}

static int Entity_enemy(lua_State *L) {
  Variant *all = Entity::enemy();
  lua_newtable(L);
  int i = 0;
  for (auto &item : all->GetArray()) {
    i++;
    entity_warp(L, &item);
    lua_rawseti(L, -2, i);
  }
  delete all;
  return 1;
}

static int Spell_all(lua_State *L) {
  Variant *all = Spell::getAll();
  lua_newtable(L);
  for (auto &prop : all->GetObject()) {
    std::string key = prop.name.GetString();
    Value item = prop.value.GetObject();
    lua_newtable(L);

    int spellId = item.GetInt("spellId");
    lua_pushinteger(L, spellId);
    lua_setfield(L, -2, "spellId");

    int common = item.GetInt("common");
    lua_pushinteger(L, common);
    lua_setfield(L, -2, "common");

    int spellType = item.GetInt("spellType");
    lua_pushinteger(L, spellType);
    lua_setfield(L, -2, "spellType");

    int materialId = item.GetInt("materialId");
    lua_pushinteger(L, materialId);
    lua_setfield(L, -2, "materialId");

    int materialCount = item.GetInt("materialCount");
    lua_pushinteger(L, materialCount);
    lua_setfield(L, -2, "materialCount");

    int useHp = item.GetInt("useHp");
    lua_pushinteger(L, useHp);
    lua_setfield(L, -2, "useHp");

    int useMp = item.GetInt("useMp");
    lua_pushinteger(L, useMp);
    lua_setfield(L, -2, "useMp");

    int learned = item.GetInt("learned");
    lua_pushinteger(L, learned);
    lua_setfield(L, -2, "learned");

    int nameId = item.GetInt("nameId");
    lua_pushinteger(L, nameId);
    lua_setfield(L, -2, "nameId");

    std::string name = item.GetString("name");
    lua_pushstring(L, name.c_str());
    lua_setfield(L, -2, "name");

    lua_setfield(L, -2, key.c_str());
  }
  delete all;
  return 1;
}


static int Spell_use(lua_State *L) {
  int spellId = luaL_checkinteger(L, 1);
  signed char isSelf = -1;
  if (lua_gettop(L) == 2)
    isSelf = lua_toboolean(L, 2);
  LOGD("spellId:%d isSelf:%d", spellId, isSelf);
  Spell::use(spellId, isSelf);
  return 0;
}


static int Actor_props(lua_State *L) {
  Variant *props = Actor::props();
  lua_newtable(L);
  for (auto &prop : props->GetObject()) {
    std::string key = prop.name.GetString();
    uint64_t value = prop.value.GetUint64();
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key.c_str());
  }
  delete props;
  return 1;
}


static int Actor_pos(lua_State *L) {
  int* pos = Actor::pos();
  lua_newtable(L);
  lua_pushinteger(L, pos[0]);
  lua_setfield(L, -2, "x");
  lua_pushinteger(L, pos[1]);
  lua_setfield(L, -2, "y");
  lua_pushinteger(L, pos[2]);
  lua_setfield(L, -2, "mapId");
  return 1;
}


static int Actor_auto(lua_State *L) {
  signed char isAuto = -1;
  if (lua_gettop(L) == 1)
    isAuto = lua_toboolean(L, 1);
  LOGD("set auto:%d", isAuto);
  char ori = Actor::automan(isAuto);
  lua_pushboolean(L, ori);
  return 1;
}


static int Actor_attach(lua_State *L) {
  signed char isAttach = -1;
  if (lua_gettop(L) == 1)
    isAttach = lua_toboolean(L, 1);
  LOGD("set attach:%d", isAttach);
  char ori = Actor::attach(isAttach);
  lua_pushboolean(L, ori);
  return 1;
}


static int Actor_self(lua_State *L) {
  signed char isSelf = -1;
  if (lua_gettop(L) == 1)
    isSelf = lua_toboolean(L, 1);
  LOGD("set self:%d", isSelf);
  char ori = Actor::self(isSelf);
  lua_pushboolean(L, ori);
  return 1;
}

static int Actor_stats(lua_State *L) {
  Variant *stats = Actor::stats();
  lua_newtable(L);
  for (auto &stat : stats->GetObject()) {
    std::string key = stat.name.GetString();
    int value = stat.value.GetInt();
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key.c_str());
  }
  delete stats;
  return 1;
}


static int Actor_selected(lua_State *L) {
  Variant* selected = Actor::selected();
  lua_pushnil(L);
  if (selected) {
    entity_warp(L, selected);
    delete selected;
  }
  return 1;
}


static int Actor_select(lua_State *L) {
  obj ptr = luaL_checkinteger(L, 1);
  Actor::select(ptr);
  return 0;
}


static int Chat_log(lua_State *L) {
  const char* log = luaL_checkstring(L, 1);
  Chat::log(log);
  return 0;
}


static int Map_describe(lua_State *L) {
  Variant* pool = Map::describe();
  lua_newtable(L);

  int worldId = pool->GetInt("worldId");
  lua_pushinteger(L, worldId);
  lua_setfield(L, -2, "worldId");

  std::string worldName = pool->GetString("worldName");
  lua_pushstring(L, worldName.c_str());
  lua_setfield(L, -2, "worldName");

  int areaId = pool->GetInt("areaId");
  lua_pushinteger(L, areaId);
  lua_setfield(L, -2, "areaId");

  std::string areaName = pool->GetString("areaName");
  lua_pushstring(L, areaName.c_str());
  lua_setfield(L, -2, "areaName");

  int tag = pool->GetInt("tag");
  lua_pushinteger(L, tag);
  lua_setfield(L, -2, "tag");

  delete pool;
  return 1;
}

static int ResourceisReady(lua_State *L) {
  bool ready = Resource::isReady();
  lua_pushboolean(L, ready);
  return 1;
}


static int xpath_gc(lua_State *L) {
  UI::Xpath *path = *(UI::Xpath **)luaL_checkudata(L, 1, "Xpath");
  LOGD("path->free:%d path:%p", path->free, path);
  if (path->free)
    delete path;
  return 0;
}


static int xpath_click(lua_State *L) {
  UI::Xpath *path = *(UI::Xpath**)luaL_checkudata(L, 1, "Xpath");
  LOGD("xpath_click:%p", path);
  UI::click(path);
  return 0;
}


static int xpath_prop(lua_State *L) {
    UI::Xpath* path = *(UI::Xpath **)luaL_checkudata(L, 1, "Xpath");
    lua_newtable(L);
    lua_pushinteger(L, path->tag);
    lua_setfield(L, -2, "tag");
    lua_pushstring(L, path->name.c_str());
    lua_setfield(L, -2, "name");
    lua_pushstring(L, path->clazz.c_str());
    lua_setfield(L, -2, "class");
    lua_pushboolean(L, path->enabled);
    lua_setfield(L, -2, "enabled");
    return 1;
}

static void xpath_warp(lua_State *L, UI::Xpath* path) {
  UI::Xpath** pXpath = (UI::Xpath**)lua_newuserdata(L, sizeof(UI::Xpath**));
  *pXpath = path;
  luaL_setmetatable(L, "Xpath");
}

static int xpath_travel(lua_State *L) {
  UI::Xpath* path = *(UI::Xpath **)luaL_checkudata(L, 1, "Xpath");
  const char* route = luaL_checkstring(L , 2);
  lua_pushnil(L);
  UI::Xpath* sub = UI::travelXpath(path, route);
  if (sub) {
    LOGD("xpath_travel find:%p", sub);
    xpath_warp(L, sub);
  }
  return 1;
}

static int xpath_enabled(lua_State *L) {
  UI::Xpath* path = *(UI::Xpath **)luaL_checkudata(L, 1, "Xpath");
  lua_pushboolean(L, path->enabled);
  return 1;
}

static int xpath_count(lua_State *L) {
  UI::Xpath* path = *(UI::Xpath **)luaL_checkudata(L, 1, "Xpath");
  int count = path->children.size();
  lua_pushinteger(L, count);
  return 1;
}

static int xpath_string(lua_State *L) {
  UI::Xpath* path = *(UI::Xpath **)luaL_checkudata(L, 1, "Xpath");
  std::string pool;
  UI::xpath2string(path, pool);
  lua_pushstring(L, pool.c_str());
  return 1;
}

static int UI_travelXpath(lua_State *L) {
  UI::Xpath** proot = (UI::Xpath**)lua_touserdata(L, 1);
  LOGD("UI_travelXpath proot:%p", proot);
  const char* route = lua_tostring(L , 2);
  if (!proot) {
    UI::Xpath* root = UI::genXpath();
    xpath_warp(L, root);
    proot = &root;
  }
  if (route) {
    lua_pushnil(L);
    UI::Xpath* sub = UI::travelXpath(*proot, route);
    if (sub) {
      LOGD("UI_travelXpath find:%p", sub);
      xpath_warp(L, sub);
    }
  }

  return 1;
}

static struct luaL_Reg inventory_fun[] = {
    {"all", InventoryAll},
    {"limit", InventoryLimit},
    {"useById", InventoryUseById},
    {"useByPtr", InventoryUseByPtr},
    {NULL, NULL}
};

static struct luaL_Reg resource_fun[] = {
  {"isReady", ResourceisReady},
  {NULL, NULL}
};

static struct luaL_Reg buff_fun[] = {
  {"all", BuffAll},
  {NULL, NULL}
};

static struct luaL_Reg card_fun[] = {
  {"all", CardAll},
  {NULL, NULL}
};

static struct luaL_Reg entity_fun[] = {
  {"all", EntityAll},
  {"enemy", Entity_enemy},
  {NULL, NULL}
};

static struct luaL_Reg spell_fun[] = {
  {"all", Spell_all},
  {"use", Spell_use},
  {NULL, NULL}
};

static struct luaL_Reg actor_fun[] = {
  {"props", Actor_props},
  {"stats", Actor_stats},
  {"pos", Actor_pos},
  {"auto", Actor_auto},
  {"attach", Actor_attach},
  {"self", Actor_self},
  {"select", Actor_select},
  {"selected", Actor_selected},
  {NULL, NULL}
};

static struct luaL_Reg ui_fun[] = {
  {"travelXpath", UI_travelXpath},
  {NULL, NULL}
};

static struct luaL_Reg chat_fun[] = {
  {"log", Chat_log},
  {NULL, NULL}
};

static struct luaL_Reg map_fun[] = {
  {"desc", Map_describe},
  {NULL, NULL}
};

static struct luaL_Reg xlib[] = {
  {"__gc", xpath_gc},
  {"__tostring", xpath_string},
  {"enabled", xpath_enabled},
  {"prop", xpath_prop},
  {"count", xpath_count},
  {"travel", xpath_travel},
  {"click", xpath_click},
  {NULL, NULL}
};

static void createXpathMeta(lua_State *L) {
  luaL_newmetatable(L, "Xpath");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, xlib, 0);
  lua_pop(L, 1);
}

int open_api(lua_State *L) {
  LOGD("注册lua api");

  lua_newtable(L);
  luaL_setfuncs(L, inventory_fun, 0);
  lua_setglobal(L, "Inventory");

  lua_newtable(L);
  luaL_setfuncs(L, buff_fun, 0);
  lua_setglobal(L, "Buff");

  lua_newtable(L);
  luaL_setfuncs(L, card_fun, 0);
  lua_setglobal(L, "Card");

  lua_newtable(L);
  luaL_setfuncs(L, entity_fun, 0);
  lua_setglobal(L, "Entity");

  lua_newtable(L);
  luaL_setfuncs(L, spell_fun, 0);
  lua_setglobal(L, "Spell");

  lua_newtable(L);
  luaL_setfuncs(L, actor_fun, 0);
  lua_setglobal(L, "Actor");

  lua_newtable(L);
  luaL_setfuncs(L, resource_fun, 0);
  lua_setglobal(L, "Resource");

  lua_newtable(L);
  luaL_setfuncs(L, chat_fun, 0);
  lua_setglobal(L, "Chat");

  lua_newtable(L);
  luaL_setfuncs(L, map_fun, 0);
  lua_setglobal(L, "Map");

  createXpathMeta(L);
  lua_newtable(L);
  luaL_setfuncs(L, ui_fun, 0);
  lua_setglobal(L, "UI");

  lua_register(L, "log", log);
  lua_register(L, "netLog", net_log);

  return 0;
}
