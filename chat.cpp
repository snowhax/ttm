
#include "chat.h"
#include "resource.h"

void Chat::log(const char* log) {
  static fun_1 print = (fun_1)(COM::alsr(FunAddrChatLog));
  std::string text = log;
  print((obj)&text);
}
