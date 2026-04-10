#ifndef TOOL_MANAGER_H_
#define TOOL_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "message.h"
#include "sandbox.h"
#include "tools/tool_factory.h"

class ToolManager {
 public:
  ToolManager();

  Json BuildToolsSchema() const;
  std::optional<ToolMessage> Execute(const Json& tool_call, std::string* error);
  std::optional<ToolMessage> Execute(const std::string& name,
                                     const Json& arguments,
                                     std::string* error);

  Sandbox& sandbox() { return sandbox_; }

 private:
  void Register(std::unique_ptr<Tool> tool);

  // Returns true if the tool's path argument passes the sandbox check.
  // For tools without a path parameter, always returns true.
  bool CheckSandbox(const std::string& tool_name,
                    const Json& arguments,
                    std::string* error);

 private:
  std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
  Sandbox sandbox_;
};

#endif  // TOOL_MANAGER_H_
