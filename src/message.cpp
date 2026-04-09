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

Json Message::ToJson() const {
  Json j;
  j["role"] = RoleToString(role);
  j["content"] = text;
  return j;
}

  return j;
}
