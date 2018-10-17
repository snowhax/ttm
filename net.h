#ifndef __NET_H__
#define __NET_H__

#include "universal.h"
#include "common.h"

typedef struct {
    uint32_t size;
    uint32_t id;
    uint32_t index;
    char buffer[];
} Packet;

namespace Net {
  int init();
  int recv(std::vector<Packet*>& v);
  int send(Variant* msg);
}

#endif
