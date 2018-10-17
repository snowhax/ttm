
#include "common.h"
#include <cxxabi.h>
#include <memory>
#include <sys/stat.h>
#include <typeinfo>

intptr_t GlibBase = 0;

std::vector<std::string> COM::split(std::string str, std::string pattern) {
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern;
    int size = str.size();

    for(int i = 0; i < size; i++) {
        pos = str.find(pattern,i);
        if(pos < size) {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() -1;
        }
    }
    return result;
}

std::string COM::demangle(const char *name) {
  int status = 1;
  std::unique_ptr<char, void (*)(void *)> res{
      abi::__cxa_demangle(name, NULL, NULL, &status), std::free};
  return (status == 0) ? res.get() : name;
}

void COM::updateConfig() {
  static const char *path = "/data/local/tmp/config.json";
  static time_t last;
  struct stat st = {0};
  int ret = lstat(path, &st);
  if (ret == -1) {
    LOGD("检测配置文件状态失败 errno:%d", errno);
    return;
  }
  if (last != st.st_mtime) {
    LOGD("开始更新配置:%lld", last);
    FILE *fp = fopen(path, "r");
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buffer = (char *)malloc(sizeof(char) * size);
    memset(buffer, 0, size);
    size_t result = fread(buffer, 1, size, fp);
    LOGD("(%ld/%ld), 配置:%s", result, size, buffer);
    fclose(fp);
    free(buffer);
    last = st.st_mtime;
  }
}

std::string COM::clsName(void *that) {
  obj rtti = r(r(that) - sizeof(void *));
  const char *name = (const char *)r(rtti + sizeof(void *));
  return demangle(name);
}

static intptr_t moduleBase(const char *name) {
  FILE *fp;
  char *pch;
  char filename[32];
  char line[1024];
  intptr_t base = 0;

  snprintf(filename, sizeof(filename), "/proc/self/maps");
  fp = fopen(filename, "r");

  if (fp != NULL) {
    while (fgets(line, sizeof(line), fp)) {
      if (strstr(line, name)) {
        pch = strtok(line, "-");
        base = strtoul(pch, NULL, 16);
        break;
      }
    }
    fclose(fp);
  }
  return base;
}

intptr_t COM::alsr(intptr_t relative) {
  if (!GlibBase)
    GlibBase = moduleBase("liblineage_sharedcpp.so");
  intptr_t target = GlibBase + relative;
  return target;
}

uint64_t COM::getBootTick() {
  int gl = r(COM::alsr(MemAddrGameLoopSystem));
  return gl ? *(uint64_t *)(gl + 0x18) : 0;
}
