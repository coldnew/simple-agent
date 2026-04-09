#include "agent.h"

#include <iostream>

#include "tools.h"

Agent::Agent(const std::string& api_url,
             const std::string& api_key,
             const std::string& model,
             const std::string& system_prompt)
    : api_url_(api_url), api_key_(api_key), model_(model) {
  if (!system_prompt.empty()) {
    messages_.push_back(SystemMessage(system_prompt).ToJson());
  }
}

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

std::string Agent::Run(const std::string& query) {
  messages_.push_back(UserMessage(query).ToJson());

  // A single user turn may include multiple model requests when tool calls
  // are returned. Keep the chain bounded to avoid infinite loops.
  const int kMaxToolRounds = 8;
  ToolManager tool_manager;
  for (int round = 0; round < kMaxToolRounds; ++round) {
    Json payload;
    payload["model"] = model_;
    payload["max_tokens"] = 1024;
    payload["tools"] = tool_manager.BuildToolsSchema();
    payload["tool_choice"] = "auto";

    payload["messages"] = messages_;

    const Json response = SendRequest(payload);

#if 0
    std::cerr << "Response (" << round <<  "): " << response.dump() << std::endl;
#endif

    std::string error;
    const std::optional<AssistantMessage> assistant_msg =
        GetAssistantMessage(response, &error);
    if (!assistant_msg.has_value()) {
      return "Error: " + error;
    }
    messages_.push_back(assistant_msg->ToJson());

    if (!assistant_msg->HasToolCalls()) {
      return assistant_msg->Text();
    }

    for (const auto& tool_call : assistant_msg->ToolCalls()) {
      const std::optional<ToolMessage> tool_msg =
          tool_manager.Execute(tool_call, &error);
      if (!tool_msg.has_value()) {
        return "Error: " + error;
      }
      messages_.push_back(tool_msg->ToJson());
    }
  }

  return "Error: Reached max tool-call rounds";
}

// A sample response from the API might look like this: (without tool calls)
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

// If we enable tool calls:

// first we send a user message that includes a request to read a file:
//
// {
//   "max_tokens": 1024,
//   "messages": [
//     {
//       "content": "You are a helpful assistant who named AI.",
//       "role": "system"
//     },
//     {
//       "content": "read file README.md",
//       "role": "user"
//     }
//   ],
//   "model": "gemma4:e4b",
//   "tool_choice": "auto",
//   "tools": [
//     {
//       "function": {
//         "description": "Read a UTF-8 text file from local path",
//         "name": "read_file",
//         "parameters": {
//           "additionalProperties": false,
//           "properties": {
//             "path": {
//               "description": "File path",
//               "type": "string"
//             }
//           },
//           "required": [
//             "path"
//           ],
//           "type": "object"
//         }
//       },
//       "type": "function"
//     }
//   ]
// }
//
// round 1 response with tool call:
//
// {
//   "choices": [
//     {
//       "finish_reason": "tool_calls",
//       "index": 0,
//       "message": {
//         "content": "",
//         "reasoning": "The user is asking to read the content of a file named
//         \"README.md\". I should use the `read_file` tool for this task and
//         pass \"README.md\" as the `path` argument.", "role": "assistant",
//         "tool_calls": [
//           {
//             "function": {
//               "arguments": "{\"path\":\"README.md\"}",
//               "name": "read_file"
//             },
//             "id": "call_1qgf3dhs",
//             "index": 0,
//             "type": "function"
//           }
//         ]
//       }
//     }
//   ],
//   "created": 1775720012,
//   "id": "chatcmpl-979",
//   "model": "gemma4:e4b",
//   "object": "chat.completion",
//   "system_fingerprint": "fp_ollama",
//   "usage": {
//     "completion_tokens": 65,
//     "prompt_tokens": 105,
//     "total_tokens": 170
//   }
// }
//
//
// we send the tool call to the `read_file` tool, which reads the
// content of `README.md` and returns it as a string. The model then
// generates a follow-up response that includes the content of the
// file and an explanation of what it did. This is reflected in the
// round 2 response below:
//
// {
//   "max_tokens": 1024,
//   "messages": [
//     {
//       "content": "You are a helpful assistant who named AI.",
//       "role": "system"
//     },
//     {
//       "content": "read file README.md",
//       "role": "user"
//     },
//     {
//       "content": null,
//       "role": "assistant",
//       "tool_calls": [
//         {
//           "function": {
//             "arguments": "{\"path\":\"README.md\"}",
//             "name": "read_file"
//           },
//           "id": "call_jeoq921j",
//           "index": 0,
//           "type": "function"
//         }
//       ]
//     },
//     {
//       "content": "simple agent\n--------------\n\nA simple AI agent in
//       c++\n\n## Features\n\n- Chat history\n- OpenAI-compatible `tools`
//       support with built-in `read_file`\n\n## Build\n\n```bash\nmkdir -p
//       build && cd $_\ncmake ..\nmake -j30\n```\n\n##
//       Usage\n\n```bash\nAPI_KEY=\"$OPENROUTER_API_KEY\"
//       API_URL=\"https://openrouter.ai/api/v1\" MODEL=\"openrouter/free\"
//       ./build/src/simple_agent\n```\n\n### Ollama (OpenAI-compatible
//       API)\n\n```bash\nAPI_KEY=\"ollama\"
//       API_URL=\"http://localhost:11434/v1\" MODEL=\"gemma4:e4b\"
//       ./build/src/simple_agent\n```\n\n### LM Studio (OpenAI-compatible
//       API)\n\n```bash\nAPI_KEY=\"lmstudio\"
//       API_URL=\"http://localhost:1234/v1\" MODEL=\"gemma4:e4b\"
//       ./build/src/simple_agent\n```\n\n## clang-format\n\nTo run
//       clang-format, use following command\n\n```bash\ncmake --build build/
//       --target format\n```\n", "name": "read_file", "role": "tool",
//       "tool_call_id": "call_jeoq921j"
//     }
//   ],
//   "model": "gemma4:e4b",
//   "tool_choice": "auto",
//   "tools": [
//     {
//       "function": {
//         "description": "Read a UTF-8 text file from local path",
//         "name": "read_file",
//         "parameters": {
//           "additionalProperties": false,
//           "properties": {
//             "path": {
//               "description": "File path",
//               "type": "string"
//             }
//           },
//           "required": [
//             "path"
//           ],
//           "type": "object"
//         }
//       },
//       "type": "function"
//     }
//   ]
// }
//
// round 2 response with tool call execution:
// {
//   "choices": [
//     {
//       "finish_reason": "stop",
//       "index": 0,
//       "message": {
//         "content": "Here is the content of the `README.md` file:\n\n***\n\n#
//         simple agent\n--------------\n\nA simple AI agent in c++\n\n##
//         Features\n\n- Chat history\n- OpenAI-compatible `tools` support with
//         built-in `read_file`\n\n## Build\n\n```bash\nmkdir -p build && cd
//         $_\ncmake ..\nmake -j30\n```\n\n## Usage\n\n```bash\n# For
//         OpenRouter\nAPI_KEY=\"$OPENROUTER_API_KEY\"
//         API_URL=\"https://openrouter.ai/api/v1\" MODEL=\"openrouter/free\"
//         ./build/src/simple_agent\n\n# For Ollama (OpenAI-compatible
//         API)\nAPI_KEY=\"ollama\" API_URL=\"http://localhost:11434/v1\"
//         MODEL=\"gemma4:e4b\" ./build/src/simple_agent\n\n# For LM Studio
//         (OpenAI-compatible API)\nAPI_KEY=\"lmstudio\"
//         API_URL=\"http://localhost:1234/v1\" MODEL=\"gemma4:e4b\"
//         ./build/src/simple_agent\n```\n\n## clang-format\n\nTo run
//         clang-format, use following command\n\n```bash\ncmake --build build/
//         --target format\n```\n\n***\n\nThis README outlines a simple AI agent
//         developed in C++, detailing its features, how to build it, and how to
//         run it with various local and remote API endpoints (OpenRouter,
//         Ollama, and LM Studio).\n\nLet me know if you have any questions
//         about this project, or if there is anything else I can help you
//         with!", "reasoning": "The user asked to read the file `README.md`,
//         and I executed the `read_file` tool with that path. The tool returned
//         the content of the `README.md` file. Now, I need to present this
//         content to the user in a readable and helpful manner. The content is
//         a README for a simple AI agent written in C++. I should summarize
//         what the file contains or just present the content clearly. Since
//         it's a technical README, presenting it formatted might be best.",
//         "role": "assistant"
//       }
//     }
//   ],
//   "created": 1775720028,
//   "id": "chatcmpl-135",
//   "model": "gemma4:e4b",
//   "object": "chat.completion",
//   "system_fingerprint": "fp_ollama",
//   "usage": {
//     "completion_tokens": 451,
//     "prompt_tokens": 391,
//     "total_tokens": 842
//   }
// }
//
std::optional<AssistantMessage> Agent::GetAssistantMessage(
    const Json& response,
    std::string* error) const {
  error->clear();

  if (response.contains("error")) {
    *error = response["error"].dump();
    return std::nullopt;
  }

  if (!response.contains("choices") || !response["choices"].is_array() ||
      response["choices"].empty()) {
    *error = "Invalid response format: " + response.dump();
    return std::nullopt;
  }

  if (!response["choices"][0].contains("message")) {
    *error = "Invalid response format: choices[0].message missing";
    return std::nullopt;
  }
  const auto& msg = response["choices"][0]["message"];
  if (!msg.is_object()) {
    *error = "Invalid response message: " + msg.dump();
    return std::nullopt;
  }

  std::string content;
  if (msg.contains("content") && msg["content"].is_string()) {
    content = msg["content"].get<std::string>();
  }

  if (msg.contains("tool_calls") && msg["tool_calls"].is_array() &&
      !msg["tool_calls"].empty()) {
    return AssistantMessage(content, msg["tool_calls"]);
  }

  return AssistantMessage(content);
}
