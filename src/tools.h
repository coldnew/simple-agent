#ifndef TOOLS_H_
#define TOOLS_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "message.h"

class Tool {
 public:
  std::string name;
  std::string description;
  Json parameters;
  std::function<std::optional<ToolMessage>(const Json& arguments,
                                           std::string* error)>
      handler;

  virtual std::string Execute(const Json& arguments, std::string* error) = 0;
};

class ToolManager {
 public:
  ToolManager();

  Json BuildToolsSchema() const;
  std::optional<ToolMessage> Execute(const Json& tool_call,
                                     std::string* error) const;
  std::optional<ToolMessage> Execute(const std::string& name,
                                     const Json& arguments,
                                     std::string* error) const;

 private:
  void Register(std::unique_ptr<Tool> tool);

 private:
  std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
};

#endif  // TOOLS_H_
