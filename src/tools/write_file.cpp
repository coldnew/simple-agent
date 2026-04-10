#include <filesystem>
#include <fstream>

#include "tool_factory.h"

namespace {

std::string WriteFileContent(const Json& arguments, std::string* error) {
  error->clear();
  if (!arguments.is_object()) {
    *error = "write_file arguments must be an object";
    return "";
  }
  if (!arguments.contains("path") || !arguments["path"].is_string()) {
    *error = "write_file requires string field: path";
    return "";
  }
  if (!arguments.contains("content") || !arguments["content"].is_string()) {
    *error = "write_file requires string field: content";
    return "";
  }

  const std::string path = arguments["path"].get<std::string>();
  const std::string content = arguments["content"].get<std::string>();

  // Create parent directories if they don't exist.
  std::filesystem::path p(path);
  if (p.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
  }

  std::ofstream ofs(path, std::ios::out | std::ios::binary);
  if (!ofs.is_open()) {
    *error = "write_file failed to open: " + path;
    return "";
  }

  ofs << content;
  if (!ofs.good()) {
    *error = "write_file failed to write: " + path;
    return "";
  }

  return "File written: " + path;
}

class WriteFileTool : public Tool {
 public:
  WriteFileTool() {
    name = "write_file";
    description = "Write content to a UTF-8 text file";
    parameters = {
        {"type", "object"},
        {"properties",
         {{"path", {{"type", "string"}, {"description", "File path"}}},
          {"content",
           {{"type", "string"}, {"description", "Content to write"}}}}},
        {"required", Json::array({"path", "content"})},
        {"additionalProperties", false}};
  }

  std::string Execute(const Json& arguments, std::string* error) override {
    return WriteFileContent(arguments, error);
  }
};

}  // namespace

std::unique_ptr<Tool> CreateWriteFileTool() {
  return std::make_unique<WriteFileTool>();
}
