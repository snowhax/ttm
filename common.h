#ifndef __COMMON_H__
#define __COMMON_H__

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "stack"
#include "universal.h"
#include "addr.h"
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <sstream>


#define MAIN_LUA_PATH "/data/local/tmp/main.lua"
#define CHERRY_CONFIG_PATH "/data/local/tmp/cherry.json"

enum Type {
  kNullType = 0,   //!< null
  kFalseType = 1,  //!< false
  kTrueType = 2,   //!< true
  kObjectType = 3, //!< object
  kArrayType = 4,  //!< array
  kStringType = 5, //!< string
  kNumberType = 6  //!< number
};

typedef int (*fun_0)(void);
typedef int (*fun_1)(int);
typedef int (*fun_2)(int, int);
typedef int (*fun_3)(int, int, int);
typedef int (*fun_4)(int, int, int, int);
typedef int (*fun_5)(int, int, int, int, int);
typedef int (*fun_6)(int, int, int, int, int, int);
typedef int (*fun_7)(int, int, int, int, int, int, int);
typedef int (*fun_8)(int, int, int, int, int, int, int, int);

typedef rapidjson::Document Variant;
typedef rapidjson::Value Value;
typedef rapidjson::Document::AllocatorType Allocator;
typedef intptr_t obj;

#define r(x) (*(obj*)(x))

#define StringRef(x) (rapidjson::StringRef(x.c_str()))

extern intptr_t GlibBase;

namespace COM {
typedef struct CCNode {
  void *_this;
  char _0x004_stuff[0x14c];
  std::vector<CCNode *> _children;
  CCNode *_parent;
  void *_director;
  int _tag;
  std::string _name;
  char _0x174_stuff[0x1c];
  bool _running;
  bool _enabled;
} CCNode;

template<typename T>
std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
std::vector<std::string> split(std::string str, std::string pattern);
std::string demangle(const char *name);
std::string clsName(void* that);
intptr_t alsr(intptr_t relative);
void updateConfig();
uint64_t getBootTick();

}

#endif
