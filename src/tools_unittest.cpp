#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "message.cpp"
#include "tool_manager.cpp"

namespace {

Json BuildToolCall(const std::string& id,
                   const std::string& name,
                   const Json& arguments) {
  Json tool_call;
  tool_call["id"] = id;
  tool_call["function"] = Json::object();
  tool_call["function"]["name"] = name;
  tool_call["function"]["arguments"] = arguments;
  return tool_call;
}

std::filesystem::path TempPath(const std::string& filename) {
  return std::filesystem::temp_directory_path() /
         ("simple_agent_" + filename + "_unittest.txt");
}

}  // namespace

struct ToolManagerSchemaTest : testing::Test {
  ToolManager tool_manager;
};

TEST_F(ToolManagerSchemaTest, ContainsReadFileSchema) {
  const Json schema = tool_manager.BuildToolsSchema();

  ASSERT_TRUE(schema.is_array());
  ASSERT_EQ(schema.size(), 1);
  EXPECT_EQ(schema[0]["type"], "function");
  EXPECT_EQ(schema[0]["function"]["name"], "read_file");
  EXPECT_EQ(schema[0]["function"]["parameters"]["type"], "object");
}

struct ExecuteToolCallTest : testing::Test {
  ToolManager tool_manager;
};

TEST_F(ExecuteToolCallTest, RejectsNonObjectPayload) {
  std::string error;
  const auto result = tool_manager.Execute(Json("bad"), &error);

  EXPECT_FALSE(result.has_value());
  EXPECT_NE(error.find("Invalid tool_call payload"), std::string::npos);
}

TEST_F(ExecuteToolCallTest, RejectsMissingId) {
  std::string error;
  Json tool_call = Json::object();
  tool_call["function"] = Json::object({{"name", "read_file"}});

  const auto result = tool_manager.Execute(tool_call, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(error, "tool_call missing id");
}

TEST_F(ExecuteToolCallTest, RejectsInvalidArgumentsJsonString) {
  std::string error;
  Json tool_call;
  tool_call["id"] = "call_1";
  tool_call["function"] = Json::object(
      {{"name", "read_file"}, {"arguments", "{this is not json}"}});

  const auto result = tool_manager.Execute(tool_call, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(error, "tool_call.arguments is not valid JSON");
}

TEST_F(ExecuteToolCallTest, RejectsUnknownToolName) {
  std::string error;
  const Json tool_call =
      BuildToolCall("call_2", "unknown_tool", Json::object());

  const auto result = tool_manager.Execute(tool_call, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(error, "Unknown tool: unknown_tool");
}

TEST_F(ExecuteToolCallTest, RejectsReadFileWithoutPath) {
  std::string error;
  const Json tool_call = BuildToolCall("call_3", "read_file", Json::object());

  const auto result = tool_manager.Execute(tool_call, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(error, "read_file requires string field: path");
}

TEST_F(ExecuteToolCallTest, ReadFileReturnsContent) {
  const std::filesystem::path path = TempPath("small");
  {
    std::ofstream ofs(path);
    ofs << "line1\nline2";
  }

  std::string error;
  const Json args = Json::object({{"path", path.string()}});
  const Json tool_call = BuildToolCall("call_4", "read_file", args);

  const auto result = tool_manager.Execute(tool_call, &error);
  ASSERT_TRUE(result.has_value()) << error;

  const Json tool_json = result->ToJson();
  EXPECT_EQ(tool_json["role"], "tool");
  EXPECT_EQ(tool_json["tool_call_id"], "call_4");
  EXPECT_EQ(tool_json["name"], "read_file");
  EXPECT_EQ(tool_json["content"], "line1\nline2");

  std::filesystem::remove(path);
}

TEST_F(ExecuteToolCallTest, ReadFileTruncatesLargeContent) {
  const std::filesystem::path path = TempPath("large");
  {
    std::ofstream ofs(path, std::ios::binary);
    ofs << std::string(70 * 1024, 'a');
  }

  std::string error;
  const Json args = Json::object({{"path", path.string()}});
  const Json tool_call = BuildToolCall("call_5", "read_file", args);

  const auto result = tool_manager.Execute(tool_call, &error);
  ASSERT_TRUE(result.has_value()) << error;

  const std::string content = result->ToJson()["content"].get<std::string>();
  EXPECT_NE(content.find("[truncated to 64KB]"), std::string::npos);

  std::filesystem::remove(path);
}
