#include <cstdlib>
#include <iostream>

#include "agent.h"

int main() {
  const char* api_url = std::getenv("API_URL");
  const char* api_key = std::getenv("API_KEY");
  const char* model = std::getenv("MODEL");

  if (!api_url || !api_key || !model) {
    std::cerr << "Please set API_URL, API_KEY, and MODEL environment variables"
              << std::endl;
    return -1;
  }

  std::string url, key, model_name;

  url = std::string(api_url) + "/chat/completions";
  key = api_key;
  model_name = model;

  std::string system_prompt = R"(You are a helpful assistant who named AI.)";

  Agent agent(url, key, model_name, system_prompt);

  std::cout << "Simple Agent - Enter your query (or 'quit' to exit):"
            << std::endl;

  while (true) {
    std::cout << "\nUser > ";
    std::string query;
    std::getline(std::cin, query);

    if (query == "quit" || query == "/quit" || query == "exit" ||
        query == "/exit") {
      break;
    }

    if (query.empty()) {
      continue;
    }

    const std::string result = agent.Run(query);
    std::cout << "\nAI > " << result << std::endl;
  }

  return 0;
}
