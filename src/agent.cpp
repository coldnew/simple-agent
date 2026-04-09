#include "agent.h"

#include <iostream>

Agent::Agent(const std::string& api_url,
             const std::string& api_key,
             const std::string& model)
    : api_url_(api_url), api_key_(api_key), model_(model) {}

Agent::~Agent() = default;

size_t Agent::WriteCallback(void* contents,
                            size_t size,
                            size_t nmemb,
                            void* userp) {
  const size_t realsize = size * nmemb;
  std::string* buffer = static_cast<std::string*>(userp);
  buffer->append(static_cast<char*>(contents), realsize);
  return realsize;
}

Json Agent::SendRequest(const Json& payload) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return Json::object({{"error", "Failed to init curl"}});
  }

  std::string response;
  struct curl_slist* headers = nullptr;

  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "Accept: application/json");

  const std::string auth_header = "Authorization: Bearer " + api_key_;
  headers = curl_slist_append(headers, auth_header.c_str());

  const std::string request_body = payload.dump();

  curl_easy_setopt(curl, CURLOPT_URL, api_url_.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                   static_cast<long>(request_body.size()));

#ifdef DEBUG
  std::cerr << "Request: " << request_body << std::endl;
#endif

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  const CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return Json::object(
        {{"error", "Failed to connect to API: " + std::to_string(res)}});
  }

  try {
    return Json::parse(response);
  } catch (...) {
    return Json::object({{"error", "Failed to parse response: " + response}});
  }
}

std::string Agent::Run(const std::string& query,
                       const std::string& system_prompt) {
  messages_.clear();

  if (!system_prompt.empty()) {
    Message sys_msg;
    sys_msg.role = Role::kSystem;
    sys_msg.content_type = ContentType::kText;
    sys_msg.text = system_prompt;
    messages_.push_back(sys_msg);
  }

  Message user_msg;
  user_msg.role = Role::kUser;
  user_msg.content_type = ContentType::kText;
  user_msg.text = query;
  messages_.push_back(user_msg);

  Json payload;
  payload["model"] = model_;
  payload["max_tokens"] = 1024;

  Json msgs = Json::array();
  for (const auto& msg : messages_) {
    msgs.push_back(MessageToJson(msg));
  }
  payload["messages"] = msgs;

  const Json response = SendRequest(payload);

  if (response.contains("error")) {
    return "Error: " + response["error"].dump();
  }

  std::string content;
  if (response.contains("choices") && !response["choices"].empty()) {
    const auto& msg = response["choices"][0]["message"];
    if (msg.contains("content") && msg["content"].is_string()) {
      content = msg["content"].get<std::string>();
    } else {
      return "Error: Response content is not a string";
    }
  } else if (response.contains("content")) {
    if (response["content"].is_string()) {
      content = response["content"].get<std::string>();
    } else {
      return "Error: Unexpected content format";
    }
  } else {
    return "Error: Invalid response format: " + response.dump();
  }

  return content;
}
