#include "message.h"

namespace {

std::string RoleToString(Role r) {
  switch (r) {
    case Role::kUser:
      return "user";
    case Role::kAssistant:
      return "assistant";
    case Role::kSystem:
      return "system";
  }
  return "user";
}

}  // namespace

Json MessageToJson(const Message& msg) {
  Json j;
  j["role"] = RoleToString(msg.role);
  j["content"] = msg.text;
  return j;
}
