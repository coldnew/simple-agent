#include <fstream>
#include <sstream>

#include "../tools.h"

namespace {

std::string ReadFileContent(const Json& arguments, std::string* error) {
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

class ReadFileTool : public Tool {
 public:
  ReadFileTool() {
    name = "read_file";
    description = "Read a UTF-8 text file from local path";
    parameters = {
        {"type", "object"},
        {"properties",
         {{"path", {{"type", "string"}, {"description", "File path"}}}}},
        {"required", Json::array({"path"})},
        {"additionalProperties", false}};
  }

  std::string Execute(const Json& arguments, std::string* error) override {
    return ReadFileContent(arguments, error);
  }
};

}  // namespace

std::unique_ptr<Tool> MakeReadFileTool() {
  return std::make_unique<ReadFileTool>();
}
