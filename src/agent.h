#ifndef AGENT_H_
#define AGENT_H_

#include <curl/curl.h>

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "message.h"

// Holds accumulated state while processing an SSE stream.
struct StreamState {
  std::string line_buffer;  // partial SSE line not yet terminated by \n
  std::string content;      // accumulated text content
  Json tool_calls = Json::array();
  bool verbose = false;
  bool done = false;
};

class Agent {
 public:
  Agent(const std::string& api_url,
        const std::string& api_key,
        const std::string& model,
        const std::string& system_prompt = "",
        bool verbose = false);
  ~Agent();

  std::string Run(const std::string& query);
  void set_verbose(bool verbose) { verbose_ = verbose; }

 private:
  friend struct AgentTest;

  std::optional<AssistantMessage> SendStreamingRequest(const Json& payload,
                                                       std::string* error);
  static size_t StreamCallback(void* contents,
                               size_t size,
                               size_t nmemb,
                               void* userp);
  static void ProcessSSELine(const std::string& line, StreamState* state);

  std::string api_url_;
  std::string api_key_;
  std::string model_;
  std::vector<Json> messages_;
  bool verbose_;
};

#endif  // AGENT_H_
