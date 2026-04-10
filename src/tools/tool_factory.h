#ifndef TOOLS_FACTORY_
#define TOOLS_FACTORY_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using Json = nlohmann::json;

class Tool {
 public:
  std::string name;
  std::string description;
  Json parameters;

  virtual std::string Execute(const Json& arguments, std::string* error) = 0;
};

std::unique_ptr<Tool> CreateReadFileTool();
std::unique_ptr<Tool> CreateWriteFileTool();
std::unique_ptr<Tool> CreateEditFileTool();
std::unique_ptr<Tool> CreateShellTool();

#endif  // TOOLS_FACTORY_
