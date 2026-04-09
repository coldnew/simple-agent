#include "tools.h"

#include <fstream>
#include <sstream>

namespace {

std::string ReadFile(const Json& arguments, std::string* error) {
  error->clear();
  if (!arguments.is_object()) {
    *error = "read_file arguments must be an object";
    return "";
  }
  if (!arguments.contains("path") || !arguments["path"].is_string()) {
    *error = "read_file requires string field: path";
    return "";
  }

  const std::string path = arguments["path"].get<std::string>();
  std::ifstream ifs(path, std::ios::in | std::ios::binary);
  if (!ifs.is_open()) {
    *error = "read_file failed to open: " + path;
    return "";
  }

  std::ostringstream oss;
  oss << ifs.rdbuf();

  static constexpr std::size_t kMaxBytes = 64 * 1024;
  std::string content = oss.str();
  if (content.size() > kMaxBytes) {
    content.resize(kMaxBytes);
    content += "\n\n[truncated to 64KB]";
  }
  return content;
}

}  // namespace

Json BuildToolsSchema() {
  Json tools = Json::array();
  tools.push_back({
      {"type", "function"},
      {"function",
       {{"name", "read_file"},
        {"description", "Read a UTF-8 text file from local path"},
        {"parameters",
         {{"type", "object"},
          {"properties",
           {{"path", {{"type", "string"}, {"description", "File path"}}}}},
          {"required", Json::array({"path"})},
          {"additionalProperties", false}}}}},
  });
  return tools;
}

std::optional<ToolMessage> ExecuteToolCall(const Json& tool_call,
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

  std::string output;
  if (name == "read_file") {
    output = ReadFile(arguments, error);
  } else {
    *error = "Unknown tool: " + name;
    return std::nullopt;
  }

  if (!error->empty()) {
    return std::nullopt;
  }

  return ToolMessage(tool_call["id"].get<std::string>(), name, output);
}
