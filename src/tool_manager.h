#ifndef TOOL_MANAGER_H_
#define TOOL_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "message.h"
#include "tools/tool_factory.h"

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

#endif  // TOOL_MANAGER_H_
