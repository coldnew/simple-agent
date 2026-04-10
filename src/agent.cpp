#include "agent.h"

#include <fmt/color.h>

#include <iostream>

#include "tool_manager.h"

Agent::Agent(const std::string& api_url,
             const std::string& api_key,
             const std::string& model,
             const std::string& system_prompt,
             bool verbose,
             double input_price,
             double output_price)
    : api_url_(api_url), api_key_(api_key), model_(model), verbose_(verbose) {
  cumulative_usage_.input_price = input_price;
  cumulative_usage_.output_price = output_price;
  if (!system_prompt.empty()) {
    messages_.push_back(SystemMessage(system_prompt).ToJson());
  }
}

Agent::~Agent() = default;

void Agent::ProcessSSELine(const std::string& line, StreamState* state) {
  // Skip empty lines and comments.
  if (line.empty() || line[0] == ':') {
    return;
  }

  // SSE lines are prefixed with "data: ".
  if (line.rfind("data: ", 0) != 0) {
    return;
  }

  const std::string data = line.substr(6);

  if (data == "[DONE]") {
    state->done = true;
    return;
  }

  Json chunk;
  try {
    chunk = Json::parse(data);
  } catch (...) {
    return;
  }

  if (state->verbose) {
    fmt::print(fg(fmt::terminal_color::bright_white), "{}\n", chunk.dump(2));
  }

  // Capture token usage (sent in the final chunk when include_usage is set).
  if (chunk.contains("usage") && chunk["usage"].is_object()) {
    const auto& u = chunk["usage"];
    state->usage.prompt_tokens = u.value("prompt_tokens", 0);
    state->usage.completion_tokens = u.value("completion_tokens", 0);
    state->usage.total_tokens = u.value("total_tokens", 0);
  }

  if (!chunk.contains("choices") || !chunk["choices"].is_array() ||
      chunk["choices"].empty()) {
    return;
  }

  const auto& delta = chunk["choices"][0].value("delta", Json::object());

  // Accumulate text content and print it in real-time.
  if (delta.contains("content") && delta["content"].is_string()) {
    const std::string text = delta["content"].get<std::string>();
    state->content += text;
    fmt::print("{}", text);
    std::cout.flush();
  }

  // Accumulate tool calls.  The first chunk for a tool call carries the id,
  // type, and function name; subsequent chunks append to function.arguments.
  if (delta.contains("tool_calls") && delta["tool_calls"].is_array()) {
    for (const auto& tc : delta["tool_calls"]) {
      const int index = tc.value("index", 0);

      // Grow the array if needed.
      while (static_cast<int>(state->tool_calls.size()) <= index) {
        state->tool_calls.push_back(
            {{"id", ""},
             {"type", "function"},
             {"function", {{"name", ""}, {"arguments", ""}}}});
      }

      auto& dest = state->tool_calls[index];
      if (tc.contains("id")) {
        dest["id"] = tc["id"];
      }
      if (tc.contains("function")) {
        if (tc["function"].contains("name")) {
          dest["function"]["name"] = tc["function"]["name"];
        }
        if (tc["function"].contains("arguments")) {
          dest["function"]["arguments"] =
              dest["function"]["arguments"].get<std::string>() +
              tc["function"]["arguments"].get<std::string>();
        }
      }
    }
  }
}

size_t Agent::StreamCallback(void* contents,
                             size_t size,
                             size_t nmemb,
                             void* userp) {
  const size_t realsize = size * nmemb;
  auto* state = static_cast<StreamState*>(userp);

  std::string chunk(static_cast<char*>(contents), realsize);
  state->line_buffer += chunk;

  // Process complete lines.
  std::string::size_type pos;
  while ((pos = state->line_buffer.find('\n')) != std::string::npos) {
    std::string line = state->line_buffer.substr(0, pos);
    state->line_buffer.erase(0, pos + 1);

    // Strip trailing \r.
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    ProcessSSELine(line, state);
  }

  return realsize;
}

std::optional<AssistantMessage> Agent::SendStreamingRequest(
    const Json& payload,
    std::string* error) {
  if (verbose_) {
    std::cout << "\n=== Send ===" << std::endl;
    fmt::print(fg(fmt::terminal_color::bright_white), "{}\n", payload.dump(2));
  }

  CURL* curl = curl_easy_init();
  if (!curl) {
    *error = "Failed to init curl";
    return std::nullopt;
  }

  StreamState state;
  state.verbose = verbose_;

  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "Accept: text/event-stream");

  const std::string auth_header = "Authorization: Bearer " + api_key_;
  headers = curl_slist_append(headers, auth_header.c_str());

  const std::string request_body = payload.dump();

  curl_easy_setopt(curl, CURLOPT_URL, api_url_.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                   static_cast<long>(request_body.size()));
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  const CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    *error = "Failed to connect to API: " + std::to_string(res);
    return std::nullopt;
  }

  // Process any remaining data in the line buffer.
  if (!state.line_buffer.empty()) {
    ProcessSSELine(state.line_buffer, &state);
  }

  if (verbose_) {
    Json accumulated;
    accumulated["content"] = state.content;
    if (!state.tool_calls.empty()) {
      accumulated["tool_calls"] = state.tool_calls;
    }
    std::cout << "\n=== Response ===" << std::endl;
    fmt::print(fg(fmt::terminal_color::bright_white), "{}\n",
               accumulated.dump(2));
  }

  // Accumulate token usage from this request.
  cumulative_usage_.prompt_tokens += state.usage.prompt_tokens;
  cumulative_usage_.completion_tokens += state.usage.completion_tokens;
  cumulative_usage_.total_tokens += state.usage.total_tokens;

  if (!state.tool_calls.empty()) {
    return AssistantMessage(state.content, state.tool_calls);
  }
  return AssistantMessage(state.content);
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
    payload["stream"] = true;
    payload["stream_options"] = {{"include_usage", true}};
    payload["tools"] = tool_manager.BuildToolsSchema();
    payload["tool_choice"] = "auto";
    payload["messages"] = messages_;

    std::string error;
    const std::optional<AssistantMessage> assistant_msg =
        SendStreamingRequest(payload, &error);
    if (!assistant_msg.has_value()) {
      return "Error: " + error;
    }
    messages_.push_back(assistant_msg->ToJson());

    if (!assistant_msg->HasToolCalls()) {
      return assistant_msg->Text();
    }

    // Newline after streamed content before tool execution output.
    if (!assistant_msg->Text().empty()) {
      std::cout << std::endl;
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

void Agent::ShowTokenUsage() const {
  const auto& usage = cumulative_usage();
  if (usage.input_price > 0 || usage.output_price > 0) {
    fmt::print(fg(fmt::terminal_color::bright_black),
               "[tokens: in={}, out={} | cost: ${:.6f} (in=${:.6f}, "
               "out=${:.6f})]\n",
               usage.prompt_tokens, usage.completion_tokens, usage.TotalCost(),
               usage.InputCost(), usage.OutputCost());
  } else {
    fmt::print(fg(fmt::terminal_color::bright_black),
               "[tokens: in={}, out={}]\n", usage.prompt_tokens,
               usage.completion_tokens);
  }
}
