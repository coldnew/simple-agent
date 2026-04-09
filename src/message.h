#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

enum class Role {
  kUser,
  kAssistant,
  kSystem,
  kTool,
};

enum class ContentType {
  kText,
};

using Json = nlohmann::json;

class IMessage {
 public:
  virtual ~IMessage() = default;
  virtual Json ToJson() const = 0;
};

// agent -> model
class UserMessage : public IMessage {
 public:
  explicit UserMessage(std::string text);
  Json ToJson() const override;

 private:
  std::string text_;
};

// agent -> model
class SystemMessage : public IMessage {
 public:
  explicit SystemMessage(std::string text);
  Json ToJson() const override;

 private:
  std::string text_;
};

// model -> agent
// {"role":"assistant","content":...,"tool_calls":[...]}
class AssistantMessage : public IMessage {
 public:
  explicit AssistantMessage(std::string text);
  AssistantMessage(std::string text, Json tool_calls);

  bool HasToolCalls() const;
  const Json& ToolCalls() const;
  const std::string& Text() const;

  Json ToJson() const override;

 private:
  std::string text_;
  Json tool_calls_;
  bool has_tool_calls_;
};

// agent -> model (tool response)
// {"role":"tool","tool_call_id":"...","name":"...","content":"..."}
class ToolMessage : public IMessage {
 public:
  ToolMessage(std::string tool_call_id, std::string name, std::string text);
  Json ToJson() const override;

 private:
  std::string tool_call_id_;
  std::string name_;
  std::string text_;
};

#endif  // MESSAGE_H_
