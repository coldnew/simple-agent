#include "tool_manager.h"

#include <unordered_set>

#include "tools/tool_factory.h"

ToolManager::ToolManager() {
  Register(CreateReadFileTool());
  Register(CreateWriteFileTool());
  Register(CreateEditFileTool());
  Register(CreateShellTool());
}

std::optional<ToolMessage> ToolManager::Execute(const Json& tool_call,
                                                std::string* error) {
  error->clear();
  if (!tool_call.is_object()) {
    *error = "Invalid tool_call payload: " + tool_call.dump();
    return std::nullopt;
  }
  if (!tool_call.contains("id") || !tool_call["id"].is_string()) {
    *error = "tool_call missing id";
    return std::nullopt;
  }
  if (!tool_call.contains("function") || !tool_call["function"].is_object()) {
    *error = "tool_call missing function object";
    return std::nullopt;
  }

  const Json& fn = tool_call["function"];
  if (!fn.contains("name") || !fn["name"].is_string()) {
    *error = "tool_call.function missing name";
    return std::nullopt;
  }

  const std::string name = fn["name"].get<std::string>();
  Json arguments = Json::object();
  if (fn.contains("arguments")) {
    if (fn["arguments"].is_string()) {
      try {
        arguments = Json::parse(fn["arguments"].get<std::string>());
      } catch (...) {
        *error = "tool_call.arguments is not valid JSON";
        return std::nullopt;
      }
    } else if (fn["arguments"].is_object()) {
      arguments = fn["arguments"];
    } else {
      *error = "tool_call.arguments must be a JSON string/object";
      return std::nullopt;
    }
  }

  auto result = Execute(name, arguments, error);
  if (!result) {
    return std::nullopt;
  }

  return ToolMessage(tool_call["id"].get<std::string>(), name, result->Text());
}

void ToolManager::Register(std::unique_ptr<Tool> tool) {
  tools_[tool->name] = std::move(tool);
}

Json ToolManager::BuildToolsSchema() const {
  Json tools = Json::array();
  for (const auto& [name, tool] : tools_) {
    tools.push_back({
        {"type", "function"},
        {"function",
         {{"name", tool->name},
          {"description", tool->description},
          {"parameters", tool->parameters}}},
    });
  }
  return tools;
}

bool ToolManager::CheckSandbox(const std::string& tool_name,
                               const Json& arguments,
                               std::string* error) {
  // File path sandbox for file tools.
  static const std::unordered_set<std::string> kFileTools = {
      "read_file", "write_file", "edit_file"};

  if (kFileTools.find(tool_name) != kFileTools.end()) {
    if (!arguments.contains("path") || !arguments["path"].is_string()) {
      return true;  // Let the tool itself report the missing-path error.
    }
    const std::string path = arguments["path"].get<std::string>();
    if (!sandbox_.CheckPathOrAsk(path)) {
      *error = "Sandbox: operation on '" + path + "' denied by user";
      return false;
    }
    return true;
  }

  // Command allowlist for shell tool.
  if (tool_name == "shell") {
    if (!arguments.contains("command") || !arguments["command"].is_string()) {
      return true;  // Let the tool itself report the missing-command error.
    }
    const std::string command = arguments["command"].get<std::string>();
    if (!sandbox_.CheckCommandOrAsk(command)) {
      *error = "Sandbox: command denied by user";
      return false;
    }
    return true;
  }

  return true;
}

std::optional<ToolMessage> ToolManager::Execute(const std::string& name,
                                                const Json& arguments,
                                                std::string* error) {
  error->clear();
  auto it = tools_.find(name);
  if (it == tools_.end()) {
    *error = "Unknown tool: " + name;
    return std::nullopt;
  }

  if (!CheckSandbox(name, arguments, error)) {
    return std::nullopt;
  }

  const std::string content = it->second->Execute(arguments, error);
  if (!error->empty()) {
    return std::nullopt;
  }
  return ToolMessage("", it->second->name, content);
}
