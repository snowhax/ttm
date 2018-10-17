
#include "json2lua.h"
#include "rapidjson/document.h"
#include "rapidjson/encodedstream.h"
#include "rapidjson/error/en.h"
#include "rapidjson/error/error.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/reader.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

struct Ctx {
  int index;
  void (*fn)(lua_State *L, Ctx *ctx);

  Ctx() : index(0), fn(NULL) {}
  explicit Ctx(void (*f)(lua_State *L, Ctx *ctx)) : index(0), fn(f) {}
  Ctx(const Ctx &rhs) : index(rhs.index), fn(rhs.fn) {}
  const Ctx &operator=(const Ctx &rhs) {
    if (this != &rhs) {
      index = rhs.index;
      fn = rhs.fn;
    }
    return *this;
  }
  static Ctx Object() { return Ctx(&objectFn); }
  static Ctx Array() { return Ctx(&arrayFn); }
  void submit(lua_State *L) {
    if (fn) fn(L, this);
  }

private:
  static void objectFn(lua_State *L, Ctx *ctx) { lua_rawset(L, -3); }
  static void arrayFn(lua_State *L, Ctx *ctx) {
    lua_rawseti(L, -2, ++ctx->index);
  }
};

struct Json2LuaHandler {
  lua_State *L;
  std::vector<Ctx> stack;
  Ctx context;

  explicit Json2LuaHandler(lua_State *GL) : L(GL) { stack.reserve(32); }

  bool Null() {
    lua_pushnil(L);
    context.submit(L);
    return true;
  }
  bool Bool(bool b) {
    lua_pushboolean(L, b);
    context.submit(L);
    return true;
  }
  bool Int(int i) {
    lua_pushinteger(L, i);
    context.submit(L);
    return true;
  }
  bool Uint(unsigned u) {
    lua_pushinteger(L, static_cast<lua_Integer>(u));
    context.submit(L);
    return true;
  }
  bool Int64(int64_t i) {
    lua_pushinteger(L, static_cast<lua_Integer>(i));
    context.submit(L);
    return true;
  }
  bool Uint64(uint64_t u) {
    lua_pushinteger(L, static_cast<lua_Integer>(u));
    context.submit(L);
    return true;
  }
  bool Double(double d) {
    lua_pushnumber(L, static_cast<lua_Number>(d));
    context.submit(L);
    return true;
  }
  bool RawNumber(const char *str, rapidjson::SizeType length, bool copy) {
    lua_getglobal(L, "tonumber");
    lua_pushlstring(L, str, length);
    lua_call(L, 1, 1);
    context.submit(L);
    return true;
  }
  bool String(const char *str, rapidjson::SizeType length, bool copy) {
    lua_pushlstring(L, str, length);
    context.submit(L);
    return true;
  }
  bool StartObject() {
    lua_createtable(L, 0, 0);
    stack.push_back(context);
    context = Ctx::Object();
    return true;
  }
  bool Key(const char *str, rapidjson::SizeType length, bool copy) const {
    lua_pushlstring(L, str, length);
    return true;
  }
  bool EndObject(rapidjson::SizeType memberCount) {
    context = stack.back();
    stack.pop_back();
    context.submit(L);
    return true;
  }
  bool StartArray() {
    lua_createtable(L, 0, 0);
    stack.push_back(context);
    context = Ctx::Array();
    return true;
  }
  bool EndArray(rapidjson::SizeType elementCount) {
    assert(elementCount == context.index);
    context = stack.back();
    stack.pop_back();
    context.submit(L);
    return true;
  }
};

template <typename Stream>
int decode(lua_State *L, Stream *s) {
  int top = lua_gettop(L);
  Json2LuaHandler handler(L);
  rapidjson::Reader reader;
  rapidjson::ParseResult r = reader.Parse(*s, handler);

  if (!r) {
    lua_settop(L, top);
    lua_pushnil(L);
    lua_pushfstring(L, "%s (%d)", GetParseError_En(r.Code()), r.Offset());
    return 2;
  }

  return 1;
}

static int json_decode(lua_State *L) {
  size_t len = 0;
  const char *contents = luaL_checklstring(L, 1, &len);
  rapidjson::StringStream s(contents);
  return decode(L, &s);
}

static const luaL_Reg methods[] = {
  {"decode", json_decode},
  {NULL, NULL}
};


int Json2Lua::decode(lua_State* L, const char* json) {
  rapidjson::StringStream s(json);
  return decode(L, &s);
}


int Json2Lua::open(lua_State *L) {
  LOGD("注册lua json");

  lua_newtable(L);
  luaL_setfuncs(L, methods, 0);
  lua_setglobal(L, "Resource");

  return 0;
}
