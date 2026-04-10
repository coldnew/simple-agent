#ifndef AGENT_H_
#define AGENT_H_

#include <curl/curl.h>

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "message.h"

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

  std::optional<AssistantMessage> GetAssistantMessage(const Json& response,
                                                      std::string* error) const;
  Json SendRequest(const Json& payload);
  static size_t WriteCallback(void* contents,
                              size_t size,
                              size_t nmemb,
                              void* userp);

  std::string api_url_;
  std::string api_key_;
  std::string model_;
  std::vector<Json> messages_;
  bool verbose_;
};

#endif  // AGENT_H_
