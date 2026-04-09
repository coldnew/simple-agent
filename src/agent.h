#ifndef AGENT_H_
#define AGENT_H_

#include <curl/curl.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "message.h"

#define DISALLOW_COPY_AND_ASSIGN(Type) \
  Type(const Type&) = delete;          \
  Type& operator=(const Type&) = delete;

class Agent {
 public:
  Agent(const std::string& api_url,
        const std::string& api_key,
        const std::string& model);
  ~Agent();

  std::string Run(const std::string& query,
                  const std::string& system_prompt = "");

 private:
  Json SendRequest(const Json& payload);
  static size_t WriteCallback(void* contents,
                              size_t size,
                              size_t nmemb,
                              void* userp);

  std::string api_url_;
  std::string api_key_;
  std::string model_;
  std::vector<Message> messages_;

  DISALLOW_COPY_AND_ASSIGN(Agent);
};

#endif  // AGENT_H_
