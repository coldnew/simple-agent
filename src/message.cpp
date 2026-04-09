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
    case Role::kTool:
      return "tool";
  }
  return "user";
}

}  // namespace

UserMessage::UserMessage(std::string text) : text_(std::move(text)) {}

Json UserMessage::ToJson() const {
  Json j;
  j["role"] = RoleToString(Role::kUser);
  j["content"] = text_;
  return j;
}

SystemMessage::SystemMessage(std::string text) : text_(std::move(text)) {}

Json SystemMessage::ToJson() const {
  Json j;
  j["role"] = RoleToString(Role::kSystem);
  j["content"] = text_;
  return j;
}

AssistantMessage::AssistantMessage(std::string text)
    : text_(std::move(text)), has_tool_calls_(false) {}

AssistantMessage::AssistantMessage(std::string text, Json tool_calls)
    : text_(std::move(text)),
      tool_calls_(std::move(tool_calls)),
      has_tool_calls_(true) {}

bool AssistantMessage::HasToolCalls() const {
  return has_tool_calls_;
}

const Json& AssistantMessage::ToolCalls() const {
  return tool_calls_;
}

const std::string& AssistantMessage::Text() const {
  return text_;
}

Json AssistantMessage::ToJson() const {
  Json j;
  j["role"] = RoleToString(Role::kAssistant);
  if (has_tool_calls_) {
    if (text_.empty()) {
      j["content"] = nullptr;
    } else {
      j["content"] = text_;
    }
    j["tool_calls"] = tool_calls_;
  } else {
    j["content"] = text_;
  }
  return j;
}

ToolMessage::ToolMessage(std::string tool_call_id,
                         std::string name,
                         std::string text)
    : tool_call_id_(std::move(tool_call_id)),
      name_(std::move(name)),
      text_(std::move(text)) {}

const std::string& ToolMessage::Text() const {
  return text_;
}

Json ToolMessage::ToJson() const {
  Json j;
  j["role"] = RoleToString(Role::kTool);
  j["content"] = text_;
  j["tool_call_id"] = tool_call_id_;
  j["name"] = name_;
  return j;
}
