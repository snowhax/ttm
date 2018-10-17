
#include "api.h"
#include "engine.h"
#include "json2lua.h"
#include "lua.hpp"
#include "net.h"
#include "ui.h"
#include "sys/system_properties.h"
#include <arpa/inet.h>
#include <sys/stat.h>
#include "actor.h"
#include "resource.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

extern int _binary___build_bundle_start;
extern int _binary___build_bundle_end;
extern int _binary___build_bundle_size;

static lua_State *GL;
Config gconfig;
static void (*MSHookFunction)(void *symbol, void *replace, void **result);

static uint64_t getTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long long ms = tv.tv_sec;
  return ms * 1000 + tv.tv_usec / 1000;
}

static int parse_config(const char *json) {
  LOGD("配置解析中...");
  Variant conf;
  conf.Parse(json);
  gconfig.guard_tick = conf.GetInt("GuardTick") ?: 1000;
  gconfig.main_tick = conf.GetInt("MainTick") ?: 3000;
  gconfig.sync_tick = conf.GetInt("SyncTick") ?: 5000;
  gconfig.is_develop = conf.GetInt("Develop") ?: 0;
  gconfig.is_paused = 1;  //默认暂停,由客户端启动
  return 0;
}

static int open_config() {
  LOGD("开始读取配置!");
  FILE *fp = fopen(CHERRY_CONFIG_PATH, "r");
  if (!fp) {
    LOGD("打开配置文件失败!");
    return -1;
  }
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *buffer = (char *)malloc(sizeof(char) * (size + 1));
  memset(buffer, 0, size + 1);
  size_t result = fread(buffer, 1, size, fp);
  LOGD("(%ld/%ld), 配置:%s", result, size, buffer);
  parse_config(buffer);
  fclose(fp);
  free(buffer);
  LOGD("配置读取完成");
  return 0;
}

static int load_lua(lua_State *GL) {
  static time_t last = 0;
  struct stat st = {0};
  int ret = lstat(MAIN_LUA_PATH, &st);
  if (ret == -1) {
    LOGD("lstat:%s 失败! errno:%d", MAIN_LUA_PATH, errno);
    return -1;
  }
  if (last != st.st_mtime) {
    last = st.st_mtime;
    int status = luaL_dofile(GL, MAIN_LUA_PATH);
    if (status != LUA_OK) {
      const char *errmsg = lua_tostring(GL, -1);
      LOGD("luaL_dofile err:%s!", errmsg);
      return -1;
    }
  }
  return 0;
}

static int lua_init(lua_State *GL) {
  const char *error = NULL;
  size_t size = 0;
  lua_getglobal(GL, "init");
  if (!lua_isfunction(GL, -1)) {
    error = "where are the init function? :(";
  } else {
    int status = lua_pcall(GL, 0, 0, 0);
    if (status != LUA_OK) {
      if (status != LUA_YIELD)
        error = lua_tolstring(GL, -1, &size);
      if (status == LUA_YIELD)
        error = "can't sleep(yield) in tick :(";
    }
  }
  if (error) {
    LOGD("main tick error:%s!", error);
    return -1;
  }
  return 0;
}

static int open_lua() {
  LOGD("初始化lua");
  GL = luaL_newstate();
  LOGD("lua_State(GL):%p!", GL);
  if (!GL)
    return -1;
  luaL_openlibs(GL);
  if (open_api(GL) < 0)
    return -1;
  if (load_lua(GL) < 0)
    return -1;
  if (lua_init(GL) < 0)
    return -1;
  return 0;
}

static void guard_tick() {
  uint64_t now = getTime();
  if (now - gconfig.guard_last > gconfig.guard_tick) {
    gconfig.guard_last = now;
    LOGD("enter guard tick:%lld", now);
    const char *error = NULL;
    size_t size = 0;
    lua_getglobal(GL, "guard");
    if (!lua_isfunction(GL, -1)) {
      error = "where are the guard tick function? :(";
    } else {
      int status = lua_pcall(GL, 0, 0, 0);
      if (status != LUA_OK) {
        if (status != LUA_YIELD)
          error = lua_tolstring(GL, -1, &size);
        if (status == LUA_YIELD)
          error = "can't sleep(yield) in tick :(";
      }
    }
    if (error)
      LOGD("guard tick error:%s!", error);
  }
}

static void main_tick() {
  uint64_t now = getTime();
  if (now - gconfig.main_last > gconfig.main_tick) {
    gconfig.main_last = now;
    LOGD("enter main tick:%lld", now);
    const char *error = NULL;
    size_t size = 0;
    lua_getglobal(GL, "main");
    if (!lua_isfunction(GL, -1)) {
      error = "where are the main function? :(";
    } else {
      int status = lua_pcall(GL, 0, 0, 0);
      if (status != LUA_OK) {
        if (status != LUA_YIELD)
          error = lua_tolstring(GL, -1, &size);
        if (status == LUA_YIELD)
          error = "can't sleep(yield) in tick :(";
      }
    }
    if (error)
      LOGD("main tick error:%s!", error);
  }
}

static void sync_actor(Value& actor, Allocator &al) {
  if (Resource::isReady()) {
    Variant *props = Actor::props();
    for (auto &prop : props->GetObject()) {
      std::string key = prop.name.GetString();
      uint64_t value = prop.value.GetUint64();
      actor.AddMember(prop.name, prop.value, al);
    }
    delete props;
  }
}

static int sync_send() {
  Variant* pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();
  pool->AddMember("cmd", "sync", al);
  Value parms(rapidjson::kObjectType);

  /* dom */
  Value dom(rapidjson::kObjectType);
  UI::Xpath* root = UI::genXpath();
  UI::xpath2json(root, dom, al);
  delXpath(root);
  parms.AddMember("dom", dom, al);

  /* actor */
  Value actor(rapidjson::kObjectType);
  sync_actor(actor, al);
  parms.AddMember("actor", actor, al);

  pool->AddMember("parms", parms, al);
  Net::send(pool);
  delete pool;
  return 0;
}

static void sync_tick() {
  uint64_t now = getTime();
  if (now - gconfig.sync_last > gconfig.sync_tick) {
    gconfig.sync_last = now;
    LOGD("enter sync tick:%lld", now);
    sync_send();
  }
}

static int open_net() { return Net::init(); }

static void (*mainloop_ori)(void *arg);
static void($mainloop)(void *arg) {
  Engine::tick();
  mainloop_ori(arg);
}

static void (*showCampaign_ori)(int r0, int r1);
static void ($showCampaign)(int r0, int r1) {}

static int open_tick() {
  LOGD("安装引擎!");
  const char *path = "/data/local/tmp/libSubstrate_arm.so";
  void *handle = dlopen(path, RTLD_NOW);
  void *MSFindSymbol = dlsym(handle, "MSFindSymbol");
  LOGD("MSFindSymbol:%p", MSFindSymbol);
  *(intptr_t *)&MSHookFunction = (intptr_t)dlsym(handle, "MSHookFunction");
  LOGD("MSHookFunction:%p", MSHookFunction);
  void *mainloop = (void *)COM::alsr(FunAddrMainloop);
  MSHookFunction(mainloop, (void *)$mainloop, (void **)&mainloop_ori);
  LOGD("$mainloop_ori:%p", mainloop_ori);

  void* showCampaign = (void*)COM::alsr(FunAddrShowCampaign);
  MSHookFunction(showCampaign, (void*)$showCampaign, (void**)&showCampaign_ori);
  LOGD("$showCampaign_ori:%p", showCampaign_ori);

  return 0;
}

static int on_control(Value *parms) {
  int paused = parms->GetInt("paused");
  LOGD("on_control paused:%d", paused);
  Engine::paused(paused);
  return 0;
}

static int update_config(std::string &json, std::string &errmsg) {
  LOGD("update_config");
  int ret = Json2Lua::decode(GL, json.c_str());
  if (ret == 2) {
    errmsg = lua_tostring(GL, -1);
    lua_pop(GL, 2);
    return -1;
  }
  if (!lua_istable(GL, -1)) {
    errmsg = "Json2Lua::decode != table";
    lua_pop(GL, 1);
    return -1;
  }
  lua_setglobal(GL, "GConfig");
  return 0;
}

static int config_ack(std::string& timestamp, int succeed, std::string &errmsg) {
  LOGD("config_ack");
  Variant *pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();
  pool->AddMember("cmd", "config.ack", al);
  Value parms(rapidjson::kObjectType);
  parms.AddMember("timestamp", timestamp, al);
  parms.AddMember("succeed", succeed, al);
  parms.AddMember("errmsg", errmsg, al);
  pool->AddMember("parms", parms, al);
  Net::send(pool);
  delete pool;
  return 0;
}

static int on_config(Value *parms) {
  LOGD("on_config");
  std::string timestamp = parms->GetString("timestamp");
  std::string json = parms->GetString("config");
  std::string errmsg;
  int succeed = update_config(json, errmsg);
  config_ack(timestamp, succeed, errmsg);
  return 0;
}

static int open_hander() {
  gconfig.handler = new std::map<std::string, eventHandler>();
  gconfig.handler->insert(std::make_pair("control", on_control));
  gconfig.handler->insert(std::make_pair("config", on_config));
  return 0;
}

Config *Engine::config() { return &gconfig; }

void Engine::login() {
  LOGD("login send!");
  Variant *pool = new Variant(rapidjson::kObjectType);
  Allocator &al = pool->GetAllocator();
  pool->AddMember("cmd", "login", al);
  Value parms(rapidjson::kObjectType);
  parms.AddMember("id", gconfig.id, al);
  pool->AddMember("parms", parms, al);
  Net::send(pool);
  delete pool;
}

int handleEvent(Packet *packet) {
  std::string json(packet->buffer, (packet->size - sizeof(Packet)));
  LOGD("recv:%s", json.c_str());
  Variant data;
  data.Parse(json.c_str());
  std::string cmd = data.GetString("cmd");
  Value parms = data.GetObject("parms");

  std::map<std::string, eventHandler>::iterator iter;
  iter = gconfig.handler->find(cmd);
  if (iter == gconfig.handler->end()) {
    LOGD("cant not get event handle:%s", cmd.c_str());
    return -1;
  }
  eventHandler callback = iter->second;
  callback(&parms);
  return 0;
}

void checkEvent() {
  std::vector<Packet *> recv;
  if (Net::recv(recv) > 0) {
    for (Packet *packet : recv) {
      handleEvent(packet);
      free(packet);
    }
  }
}

void Engine::tick() {
  checkEvent();
  if (!gconfig.is_paused) {
    if (gconfig.is_develop)
      load_lua(GL);
    guard_tick();
    main_tick();
    sync_tick();
  }
}

int Engine::paused(int op) {
  int ori = gconfig.is_paused;
  if (op != -1) {
    LOGD("gconfig.is_paused:%d", op);
    gconfig.is_paused = op;
  }
  return ori;
}

static int open_token() {
  LOGD("初始化token");
  char token[PROP_VALUE_MAX] = {0};
  __system_property_get("ttm.token", token);
  LOGD("token:%s", token);
  if (!strlen(token))
    return -1;
  std::vector<std::string> v = COM::split(token, "-");
  LOGD("v size:%d", v.size());
  if (v.size() != 2)
    return -1;
  gconfig.id = std::atoi(v[0].c_str());
  gconfig.token = new std::string(v[1]);
  LOGD("id:%d", gconfig.id);
  LOGD("token:%s", gconfig.token->c_str());
  return 0;
}

int Engine::init() {
  int size = (int)&_binary___build_bundle_size;
  char* data = (char*)&_binary___build_bundle_start;
  char* end = (char*)&_binary___build_bundle_end;

  LOGD("size:%d", size);
  LOGD("data:%s", data);

  if (open_token() < 0)
    return -1;
  if (open_config() < 0)
    return -1;
  if (open_hander() < 0)
    return -1;
  if (open_net() < 0)
    return -1;
  if (open_lua() < 0)
    return -1;
  if (open_tick() < 0)
    return -1;

  Engine::login();
  return 0;
}
