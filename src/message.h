#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <nlohmann/json.hpp>
#include <string>

enum class Role {
  kUser,
  kAssistant,
  kSystem,
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

struct Message : public IMessage {
  Role role;
  ContentType content_type;
  std::string text;

  Message() : role(Role::kUser), content_type(ContentType::kText) {}
  explicit Message(Role r) : role(r), content_type(ContentType::kText) {}

  Json ToJson() const override;
};

using Json = nlohmann::json;

Json MessageToJson(const Message& msg);

#endif  // MESSAGE_H_
