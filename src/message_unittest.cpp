#include "message.cpp"

#include <gtest/gtest.h>

struct UserMessageTest : testing::Test {};

TEST_F(UserMessageTest, ToJsonFormatsRoleAndContent) {
  const UserMessage msg("hello");
  const Json j = msg.ToJson();

  EXPECT_EQ(j["role"], "user");
  EXPECT_EQ(j["content"], "hello");
}

struct SystemMessageTest : testing::Test {};

TEST_F(SystemMessageTest, ToJsonFormatsRoleAndContent) {
  const SystemMessage msg("system prompt");
  const Json j = msg.ToJson();

  EXPECT_EQ(j["role"], "system");
  EXPECT_EQ(j["content"], "system prompt");
}

struct AssistantMessageTest : testing::Test {};

TEST_F(AssistantMessageTest, PlainTextMessageHasNoToolCalls) {
  const AssistantMessage msg("answer");
  const Json j = msg.ToJson();

  EXPECT_FALSE(msg.HasToolCalls());
  EXPECT_EQ(msg.Text(), "answer");
  EXPECT_EQ(j["role"], "assistant");
  EXPECT_EQ(j["content"], "answer");
  EXPECT_FALSE(j.contains("tool_calls"));
}

TEST_F(AssistantMessageTest,
       ToolCallMessageStoresToolCallsAndNullContentWhenEmpty) {
  const Json tool_calls = Json::array(
      {{{"id", "call_1"},
        {"type", "function"},
        {"function", {{"name", "read_file"}, {"arguments", "{}"}}}}});

  const AssistantMessage msg("", tool_calls);
  const Json j = msg.ToJson();

  EXPECT_TRUE(msg.HasToolCalls());
  EXPECT_EQ(msg.Text(), "");
  EXPECT_EQ(msg.ToolCalls(), tool_calls);
  EXPECT_EQ(j["role"], "assistant");
  EXPECT_TRUE(j["content"].is_null());
  ASSERT_TRUE(j.contains("tool_calls"));
  EXPECT_EQ(j["tool_calls"], tool_calls);
}

struct ToolMessageTest : testing::Test {};

TEST_F(ToolMessageTest, ToJsonFormatsToolResponse) {
  const ToolMessage msg("call_9", "read_file", "file content");
  const Json j = msg.ToJson();

  EXPECT_EQ(j["role"], "tool");
  EXPECT_EQ(j["tool_call_id"], "call_9");
  EXPECT_EQ(j["name"], "read_file");
  EXPECT_EQ(j["content"], "file content");
}
