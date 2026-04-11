#include "tool_manager.cpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "message.cpp"
#include "permission.cpp"
#include "tools/read_file.cpp"
#include "tools/shell.cpp"
#include "tools/write_file.cpp"

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

struct ToolsSchemaTest : testing::Test {
  ToolManager tool_manager;
};

TEST_F(ToolsSchemaTest, ContainsReadFileSchema) {
  const Json schema = tool_manager.BuildToolsSchema();

  ASSERT_TRUE(schema.is_array());
  ASSERT_EQ(schema.size(), 4);

  bool found_read_file = false;
  bool found_write_file = false;
  bool found_edit_file = false;
  bool found_bash = false;
  for (const auto& tool : schema) {
    if (tool["function"]["name"] == "read_file") {
      found_read_file = true;
      EXPECT_EQ(tool["type"], "function");
      EXPECT_EQ(tool["function"]["parameters"]["type"], "object");
    } else if (tool["function"]["name"] == "write_file") {
      found_write_file = true;
    } else if (tool["function"]["name"] == "edit_file") {
      found_edit_file = true;
    } else if (tool["function"]["name"] == "shell") {
      found_bash = true;
    }
  }
  EXPECT_TRUE(found_read_file) << "read_file not found in schema";
  EXPECT_TRUE(found_write_file) << "write_file not found in schema";
  EXPECT_TRUE(found_edit_file) << "edit_file not found in schema";
  EXPECT_TRUE(found_bash) << "bash not found in schema";
}

TEST_F(ToolsSchemaTest, ContainsWriteFileSchema) {
  const Json schema = tool_manager.BuildToolsSchema();

  ASSERT_TRUE(schema.is_array());
  ASSERT_EQ(schema.size(), 4);

  bool found_write_file = false;
  bool found_edit_file = false;
  for (const auto& tool : schema) {
    if (tool["function"]["name"] == "write_file") {
      found_write_file = true;
      EXPECT_EQ(tool["type"], "function");
      EXPECT_EQ(tool["function"]["parameters"]["type"], "object");
    } else if (tool["function"]["name"] == "edit_file") {
      found_edit_file = true;
      EXPECT_EQ(tool["function"]["parameters"]["properties"].contains("path"),
                true);
      EXPECT_EQ(
          tool["function"]["parameters"]["properties"].contains("oldString"),
          true);
      EXPECT_EQ(
          tool["function"]["parameters"]["properties"].contains("newString"),
          true);
    }
  }
  EXPECT_TRUE(found_write_file) << "write_file not found in schema";
  EXPECT_TRUE(found_edit_file) << "edit_file not found in schema";
}

struct ExecuteToolCallTest : testing::Test {
  ToolManager tool_manager{true};  // skip_permissions to avoid path validation
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

TEST_F(ExecuteToolCallTest, RejectsWriteFileWithoutPath) {
  std::string error;
  const Json tool_call = BuildToolCall("call_6", "write_file", Json::object());

  const auto result = tool_manager.Execute(tool_call, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(error, "write_file requires string field: path");
}

TEST_F(ExecuteToolCallTest, RejectsWriteFileWithoutContent) {
  std::string error;
  const Json tool_call = BuildToolCall(
      "call_7", "write_file", Json::object({{"path", "/tmp/test.txt"}}));

  const auto result = tool_manager.Execute(tool_call, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(error, "write_file requires string field: content");
}

TEST_F(ExecuteToolCallTest, WriteFileCreatesFile) {
  const std::filesystem::path path = TempPath("write_test");
  std::filesystem::remove(path);

  std::string error;
  const Json args =
      Json::object({{"path", path.string()}, {"content", "hello world"}});
  const Json tool_call = BuildToolCall("call_8", "write_file", args);

  const auto result = tool_manager.Execute(tool_call, &error);
  ASSERT_TRUE(result.has_value()) << error;

  const Json tool_json = result->ToJson();
  EXPECT_EQ(tool_json["role"], "tool");
  EXPECT_EQ(tool_json["name"], "write_file");
  EXPECT_NE(tool_json["content"].get<std::string>().find("File written"),
            std::string::npos);

  std::filesystem::remove(path);
}

struct PermissionToolTest : testing::Test {
  ToolManager tool_manager;

  void SetUp() override {
    working_dir_ =
        std::filesystem::temp_directory_path() / "permission_tool_test";
    std::filesystem::create_directories(working_dir_);
    {
      std::ofstream(working_dir_ / "allowed.txt") << "ok";
    }

    tool_manager.permission() = Permission(working_dir_.string());
    // Auto-deny all out-of-permission requests.
    tool_manager.permission().set_confirm_fn(
        [](const std::string&, const std::string&) {
          return Permission::Answer::kNo;
        });
  }

  void TearDown() override { std::filesystem::remove_all(working_dir_); }

  std::filesystem::path working_dir_;
};

TEST_F(PermissionToolTest, AllowsReadInsidePermission) {
  std::string error;
  const Json args =
      Json::object({{"path", (working_dir_ / "allowed.txt").string()}});
  const auto result = tool_manager.Execute("read_file", args, &error);
  ASSERT_TRUE(result.has_value()) << error;
  EXPECT_EQ(result->Text(), "ok");
}

TEST_F(PermissionToolTest, DeniesReadOutsidePermission) {
  std::string error;
  const Json args = Json::object({{"path", "/etc/hostname"}});
  const auto result = tool_manager.Execute("read_file", args, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(error.find("Permission"), std::string::npos);
}

TEST_F(PermissionToolTest, DeniesWriteOutsidePermission) {
  std::string error;
  const Json args =
      Json::object({{"path", "/tmp/permission_evil.txt"}, {"content", "bad"}});
  const auto result = tool_manager.Execute("write_file", args, &error);
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(error.find("Permission"), std::string::npos);
}

TEST_F(PermissionToolTest, ShellToolSkipsPermissionCheck) {
  // This test verifies that shell commands bypass permission checks
  // when skip_permissions=true is used.
  ToolManager tool_manager{true};  // skip_permissions=true
  std::string error;
  const Json args = Json::object({{"command", "echo hello"}});
  const auto result = tool_manager.Execute("shell", args, &error);
  ASSERT_TRUE(result.has_value()) << error;
  EXPECT_EQ(result->Text(), "hello");
}
