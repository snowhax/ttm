
#include "net.h"
#include "resource.h"
#include "sys/system_properties.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <uv.h>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "engine.h"
#include <mutex>

struct {
  uv_barrier_t blocker;
  uv_tcp_t* client;
  uint32_t  id;
  uint32_t  index;
  std::mutex lock;
  std::string buffer;
  std::vector<Packet*> recv;
} gnet;


typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

static void alloc_buffer(uv_handle_t *handle, size_t need_size, uv_buf_t *buf) {
  buf->base = (char *)malloc(need_size);
  buf->len = need_size;
}

static
void on_close(uv_handle_t* handle) {
  LOGD("client close!");
  Engine::paused(1);
  exit(0);
}

static
void on_write(uv_write_t *req, int status) {
  LOGD("on_write:%d", status);
  if (status) {
    LOGD("client close in write, reason:%s\n", uv_err_name(status));
    uv_close((uv_handle_t*)gnet.client, on_close);
    gnet.client = NULL;
  }
  free_write_req(req);
}


static
void append_packet(Packet* header) {
  LOGD("enter append_packet!");

  Packet* packet = (Packet*)malloc(header->size);
  LOGD("packet size:%d", header->size);
  memcpy(packet, header, header->size);
  gnet.recv.push_back(packet);

  LOGD("leave append_packet!");
}


static
bool resolve_packet() {
  if (gnet.buffer.size() < sizeof(Packet))
    return false;

  Packet* header = (Packet*)gnet.buffer.data();
  if (gnet.buffer.size() < header->size)
    return false;

  LOGD("header->size:%d gnet.buffer.size:%d", header->size, gnet.buffer.size());

  append_packet(header);
  gnet.buffer.erase(0, header->size);
  LOGD("erase gnet.buffer.size:%d", gnet.buffer.size());

  return resolve_packet();
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    LOGD("client close in read, reason:%s\n", uv_err_name(nread));
    uv_close((uv_handle_t*)gnet.client, on_close);
    gnet.client = NULL;
  } else {
    gnet.buffer.append(buf->base, nread);
    LOGD("recv:[%d] buffer.size:[%d]", nread, gnet.buffer.size());
    LOGD("enter resolve_packet!");
    gnet.lock.lock();
    resolve_packet();
    gnet.lock.unlock();
    LOGD("leave resolve_packet!");
  }
  free(buf->base);
}

static void on_connect(uv_connect_t *req, int status) {
  if (status < 0) {
    LOGD("connect failed error %s\n", uv_err_name(status));
    free(gnet.client);
    gnet.client = NULL;
  } else {
    LOGD("connect succeed!");
    uv_read_start((uv_stream_t *)req->handle, alloc_buffer, on_read);
  }
  uv_barrier_wait(&gnet.blocker);
  free(req);
}

static void entry(void *p) {
  uv_loop_t *loop = uv_default_loop();
  struct sockaddr_in addr;
  char host[PROP_VALUE_MAX] = {0};
  __system_property_get("dhcp.eth0.server", host);
  LOGD("eth0 host:%s", host);
  if (strlen(host) == 0) {
    __system_property_get("dhcp.eth1.server", host);
    LOGD("eth1 host:%s", host);
  }
  uv_ip4_addr(host, 3000, &addr);
  uv_connect_t *req = (uv_connect_t *)malloc(sizeof(uv_connect_t));
  gnet.client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, gnet.client);
  uv_tcp_connect(req, gnet.client, (const struct sockaddr *)&addr, on_connect);
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

int Net::init() {
  Config* config = Engine::config();
  gnet.id = config->id;
  uv_barrier_init(&gnet.blocker, 2);
  uv_thread_t thread;
  uv_thread_create(&thread, entry, NULL);
  uv_barrier_wait(&gnet.blocker);
  uv_barrier_destroy(&gnet.blocker);
  if (gnet.client == NULL) {
    LOGD("Net::init fail!");
    return -1;
  }
  return 0;
}

static
Packet* create_packet(Variant* msg) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  msg->Accept(writer);
  uint32_t size = sizeof(Packet) + buffer.GetSize();
  Packet* packet = (Packet*) malloc(size);
  packet->id = gnet.id;
  packet->size = size;
  packet->index = gnet.index++;
  memcpy(packet->buffer, buffer.GetString(), buffer.GetSize());
  return packet;
}

int Net::send(Variant* msg) {
  if (!gnet.client) return -1;
  write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
  Packet* packet = create_packet(msg);
  req->buf = uv_buf_init((char*)packet, packet->size);
  uv_write((uv_write_t*)req, (uv_stream_t *)gnet.client, &req->buf, 1, on_write);
  return 0;
}

int Net::recv(std::vector<Packet*>& v) {
  if (!gnet.client) return -1;

  if (gnet.lock.try_lock()) {
    //LOGD("enter Net::recv!");
    gnet.recv.swap(v);
    gnet.lock.unlock();
    //LOGD("leave Net::recv!");
  }

  return v.size();
}
