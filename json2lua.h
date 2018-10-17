#ifndef __JSON2LUA_H__
#define __JSON2LUA_H__

#include "universal.h"
#include "common.h"
#include "lua.hpp"


namespace Json2Lua {
  int open(lua_State* L);
  int decode(lua_State* L, const char* json);
}

#endif
