#include <cstdlib>
#include <sstream>

#include "tool_factory.h"

namespace {

std::string RunBashCommand(const Json& arguments, std::string* error) {
  error->clear();
  if (!arguments.is_object()) {
    *error = "bash arguments must be an object";
    return "";
  }
  if (!arguments.contains("command") || !arguments["command"].is_string()) {
    *error = "bash requires string field: command";
    return "";
  }

  const std::string command = arguments["command"].get<std::string>();
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) {
    *error = "bash failed to execute: " + command;
    return "";
  }

  std::ostringstream oss;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    oss << buffer;
  }

  int status = pclose(pipe);
  std::string output = oss.str();
  if (!output.empty() && output.back() == '\n') {
    output.pop_back();
  }

  return output;
}

class ShellTool : public Tool {
 public:
  ShellTool() {
    name = "shell";
    description = "Execute a shell command and return its output";
    parameters = {
        {"type", "object"},
        {"properties",
         {{"command",
           {{"type", "string"}, {"description", "Shell command to execute"}}}}},
        {"required", Json::array({"command"})},
        {"additionalProperties", false}};
  }

  std::string Execute(const Json& arguments, std::string* error) override {
    return RunBashCommand(arguments, error);
  }
};

}  // namespace

std::unique_ptr<Tool> CreateShellTool() {
  return std::make_unique<ShellTool>();
}
