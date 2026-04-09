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

  // A simple request payload might look like this:
  //
  //  {
  //  "max_tokens": 1024,
  //  "messages": [
  //    {
  //      "content": "hi",
  //      "role": "user"
  //    }
  //  ],
  //  "model": "gemma4:e4b"
  // }

#if 0
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
  // system prompt only added for the first message, if messages_ is empty
  if (!system_prompt.empty() && messages_.empty()) {
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

#if 0
  std::cerr << "Response: " << response.dump() << std::endl;
#endif

  const std::string content = GetResponseContent(response);

  // update messages with assistant response so that the next query
  // will have the full conversation history
  Message assistant_msg;
  assistant_msg.role = Role::kAssistant;
  assistant_msg.content_type = ContentType::kText;
  assistant_msg.text = content;
  messages_.push_back(assistant_msg);

  return content;
}

// A sample response from the API might look like this:
//
// {
//   "choices": [
//     {
//       "finish_reason": "stop",
//       "index": 0,
//       "logprobs": null,
//       "message": {
//         "content": "Hi there! How can I help you today?",
//         "reasoning": "The user has simply said \"hi\" - this is a greeting. I
//         should respond in a friendly, welcoming manner and offer to help them
//         with whatever they need.\n", "reasoning_details": [
//           {
//             "format": "unknown",
//             "index": 0,
//             "text": "The user has simply said \"hi\" - this is a greeting. I
//             should respond in a friendly, welcoming manner and offer to help
//             them with whatever they need.\n", "type": "reasoning.text"
//           }
//         ],
//         "refusal": null,
//         "role": "assistant"
//       },
//       "native_finish_reason": "stop"
//     }
//   ],
//   "created": 1775715588,
//   "id": "gen-1775715587-F2085HsLqclblKPhRFy0",
//   "model": "minimax/minimax-m2.5-20260211:free",
//   "object": "chat.completion",
//   "provider": "OpenInference",
//   "system_fingerprint": null,
//   "usage": {
//     "completion_tokens": 48,
//     "completion_tokens_details": {
//       "audio_tokens": 0,
//       "image_tokens": 0,
//       "reasoning_tokens": 38
//     },
//     "cost": 0,
//     "cost_details": {
//       "upstream_inference_completions_cost": 0,
//       "upstream_inference_cost": 0,
//       "upstream_inference_prompt_cost": 0
//     },
//     "is_byok": false,
//     "prompt_tokens": 39,
//     "prompt_tokens_details": {
//       "audio_tokens": 0,
//       "cache_write_tokens": 0,
//       "cached_tokens": 0,
//       "video_tokens": 0
//     },
//     "total_tokens": 87
//   }
// }
//
std::string Agent::GetResponseContent(const Json& response) {
  if (response.contains("error")) {
    return "Error: " + response["error"].dump();
  }

  if (!response.contains("choices") || !response["choices"].is_array() ||
      response["choices"].empty()) {
    return "Error: Invalid response format: " + response.dump();
  }

  const auto& msg = response["choices"][0]["message"];
  if (!msg.contains("content") || !msg["content"].is_string()) {
    return "Error: Response content is not a string";
  }

  return msg["content"].get<std::string>();
}
