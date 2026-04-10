#ifndef AGENT_H_
#define AGENT_H_

#include <curl/curl.h>

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "message.h"

// Token usage and cost tracking.
struct TokenUsage {
  int prompt_tokens = 0;
  int completion_tokens = 0;
  int total_tokens = 0;

  // Price per million tokens (USD). Zero means cost display is skipped.
  double input_price = 0.0;
  double output_price = 0.0;

  double InputCost() const { return prompt_tokens * input_price / 1e6; }
  double OutputCost() const { return completion_tokens * output_price / 1e6; }
  double TotalCost() const { return InputCost() + OutputCost(); }
};

class Agent {
 public:
  Agent(const std::string& api_url,
        const std::string& api_key,
        const std::string& model,
        const std::string& system_prompt = "",
        bool verbose = false,
        double input_price = 0.0,
        double output_price = 0.0);
  ~Agent();

  std::string Run(const std::string& query);
  void set_verbose(bool verbose) { verbose_ = verbose; }
  const TokenUsage& cumulative_usage() const { return cumulative_usage_; }

  void ShowTokenUsage() const;

 private:
  friend struct AgentTest;

  std::optional<AssistantMessage> GetAssistantMessage(const Json& response,
                                                      std::string* error,
                                                      TokenUsage& usage) const;
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
  TokenUsage cumulative_usage_;
};

#endif  // AGENT_H_
