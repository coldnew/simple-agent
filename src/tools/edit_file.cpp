#include <fstream>
#include <sstream>

#include "tool_factory.h"

namespace {

std::string EditFileContent(const Json& arguments, std::string* error) {
  error->clear();
  if (!arguments.is_object()) {
    *error = "edit_file arguments must be an object";
    return "";
  }
  if (!arguments.contains("path") || !arguments["path"].is_string()) {
    *error = "edit_file requires string field: path";
    return "";
  }
  if (!arguments.contains("oldString") || !arguments["oldString"].is_string()) {
    *error = "edit_file requires string field: oldString";
    return "";
  }
  if (!arguments.contains("newString") || !arguments["newString"].is_string()) {
    *error = "edit_file requires string field: newString";
    return "";
  }

  const std::string path = arguments["path"].get<std::string>();
  const std::string old_str = arguments["oldString"].get<std::string>();
  const std::string new_str = arguments["newString"].get<std::string>();

  std::ifstream ifs(path, std::ios::in | std::ios::binary);
  if (!ifs.is_open()) {
    *error = "edit_file failed to open: " + path;
    return "";
  }

  std::ostringstream oss;
  oss << ifs.rdbuf();
  std::string content = oss.str();

  std::size_t pos = content.find(old_str);
  if (pos == std::string::npos) {
    *error = "edit_file: oldString not found in file";
    return "";
  }

  content.replace(pos, old_str.length(), new_str);

  std::ofstream ofs(path, std::ios::out | std::ios::binary);
  if (!ofs.is_open()) {
    *error = "edit_file failed to write: " + path;
    return "";
  }

  ofs << content;
  if (!ofs.good()) {
    *error = "edit_file failed to write: " + path;
    return "";
  }

  return "File edited: " + path;
}

class EditFileTool : public Tool {
 public:
  EditFileTool() {
    name = "edit_file";
    description = "Replace text in an existing file";
    parameters = {
        {"type", "object"},
        {"properties",
         {{"path", {{"type", "string"}, {"description", "File path"}}},
          {"oldString",
           {{"type", "string"}, {"description", "Text to find and replace"}}},
          {"newString",
           {{"type", "string"}, {"description", "Replacement text"}}}}},
        {"required", Json::array({"path", "oldString", "newString"})},
        {"additionalProperties", false}};
  }

  std::string Execute(const Json& arguments, std::string* error) override {
    return EditFileContent(arguments, error);
  }
};

}  // namespace

std::unique_ptr<Tool> CreateEditFileTool() {
  return std::make_unique<EditFileTool>();
}
