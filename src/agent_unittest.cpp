#include "agent.cpp"

#include <gtest/gtest.h>

#include "message.cpp"
#include "sandbox.cpp"
#include "tool_manager.cpp"

struct AgentTest : testing::Test {
  Agent agent{"http://localhost", "key", "model"};

  static void ProcessSSELine(const std::string& line, StreamState* state) {
    Agent::ProcessSSELine(line, state);
  }
};

// --- SSE streaming tests ---

TEST_F(AgentTest, SSEAccumulatesTextContent) {
  StreamState state;

  ProcessSSELine(
      R"(data: {"choices":[{"delta":{"role":"assistant","content":"Hello"}}]})",
      &state);
  ProcessSSELine(R"(data: {"choices":[{"delta":{"content":" world"}}]})",
                 &state);

  EXPECT_EQ(state.content, "Hello world");
  EXPECT_TRUE(state.tool_calls.empty());
  EXPECT_FALSE(state.done);
}

TEST_F(AgentTest, SSEHandlesDone) {
  StreamState state;

  ProcessSSELine(R"(data: {"choices":[{"delta":{"content":"hi"}}]})", &state);
  ProcessSSELine("data: [DONE]", &state);

  EXPECT_EQ(state.content, "hi");
  EXPECT_TRUE(state.done);
}

TEST_F(AgentTest, SSEIgnoresEmptyAndCommentLines) {
  StreamState state;

  ProcessSSELine("", &state);
  ProcessSSELine(": this is a comment", &state);
  ProcessSSELine("event: message", &state);

  EXPECT_TRUE(state.content.empty());
  EXPECT_FALSE(state.done);
}

TEST_F(AgentTest, SSEAccumulatesToolCalls) {
  StreamState state;

  // First chunk: tool call with id and function name.
  ProcessSSELine(
      R"(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"id":"call_1","type":"function","function":{"name":"read_file","arguments":""}}]}}]})",
      &state);

  // Second chunk: arguments fragment.
  ProcessSSELine(
      R"(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"function":{"arguments":"{\"path\":"}}]}}]})",
      &state);

  // Third chunk: rest of arguments.
  ProcessSSELine(
      R"(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\"README.md\"}"}}]}}]})",
      &state);

  ASSERT_EQ(state.tool_calls.size(), 1);
  EXPECT_EQ(state.tool_calls[0]["id"], "call_1");
  EXPECT_EQ(state.tool_calls[0]["function"]["name"], "read_file");
  EXPECT_EQ(state.tool_calls[0]["function"]["arguments"],
            R"({"path":"README.md"})");
}

TEST_F(AgentTest, SSEHandlesMultipleToolCalls) {
  StreamState state;

  ProcessSSELine(
      R"(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"id":"call_1","type":"function","function":{"name":"read_file","arguments":"{}"}}]}}]})",
      &state);
  ProcessSSELine(
      R"(data: {"choices":[{"delta":{"tool_calls":[{"index":1,"id":"call_2","type":"function","function":{"name":"write_file","arguments":"{}"}}]}}]})",
      &state);

  ASSERT_EQ(state.tool_calls.size(), 2);
  EXPECT_EQ(state.tool_calls[0]["id"], "call_1");
  EXPECT_EQ(state.tool_calls[1]["id"], "call_2");
  EXPECT_EQ(state.tool_calls[1]["function"]["name"], "write_file");
}
