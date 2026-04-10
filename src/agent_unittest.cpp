#include "agent.cpp"

#include <gtest/gtest.h>

#include "message.cpp"
#include "sandbox.cpp"
#include "skill.cpp"
#include "tool_manager.cpp"

struct AgentTest : testing::Test {
  Agent agent{"http://localhost", "key", "model", "", "skills"};

  std::optional<AssistantMessage> GetAssistantMessage(const Json& response,
                                                      std::string* error) {
    TokenUsage usage;
    return agent.GetAssistantMessage(response, error, usage);
  }
};

TEST_F(AgentTest, ReturnsErrorFieldWhenResponseHasError) {
  std::string error;
  const auto msg =
      GetAssistantMessage(Json::object({{"error", "boom"}}), &error);

  EXPECT_FALSE(msg.has_value());
  EXPECT_EQ(error, "\"boom\"");
}

TEST_F(AgentTest, RejectsMissingChoicesArray) {
  std::string error;
  const auto msg =
      GetAssistantMessage(Json::object({{"id", "resp_1"}}), &error);

  EXPECT_FALSE(msg.has_value());
  EXPECT_NE(error.find("Invalid response format"), std::string::npos);
}

TEST_F(AgentTest, RejectsMissingMessageObject) {
  std::string error;
  const Json response =
      Json::object({{"choices", Json::array({Json::object()})}});
  const auto msg = GetAssistantMessage(response, &error);

  EXPECT_FALSE(msg.has_value());
  EXPECT_EQ(error, "Invalid response format: choices[0].message missing");
}

TEST_F(AgentTest, RejectsNonObjectMessage) {
  std::string error;
  const Json response = Json::object(
      {{"choices", Json::array({Json::object({{"message", "not_object"}})})}});
  const auto msg = GetAssistantMessage(response, &error);

  EXPECT_FALSE(msg.has_value());
  EXPECT_NE(error.find("Invalid response message"), std::string::npos);
}

TEST_F(AgentTest, ParsesPlainAssistantMessage) {
  std::string error;
  const Json response = Json::object(
      {{"choices", Json::array({Json::object(
                       {{"message", Json::object({{"content", "ok"}})}})})}});

  const auto msg = GetAssistantMessage(response, &error);
  ASSERT_TRUE(msg.has_value()) << error;

  EXPECT_FALSE(msg->HasToolCalls());
  EXPECT_EQ(msg->Text(), "ok");
}

TEST_F(AgentTest, ParsesMessageWithToolCalls) {
  std::string error;
  const Json tool_calls = Json::array(
      {{{"id", "call_1"},
        {"type", "function"},
        {"function",
         Json::object({{"name", "read_file"},
                       {"arguments", "{\\\"path\\\":\\\"README.md\\\"}"}})}}});
  const Json response = Json::object(
      {{"choices",
        Json::array({Json::object(
            {{"message", Json::object({{"content", ""},
                                       {"tool_calls", tool_calls}})}})})}});

  const auto msg = GetAssistantMessage(response, &error);
  ASSERT_TRUE(msg.has_value()) << error;

  EXPECT_TRUE(msg->HasToolCalls());
  EXPECT_EQ(msg->ToolCalls(), tool_calls);
}
