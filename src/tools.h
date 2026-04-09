#ifndef TOOLS_H_
#define TOOLS_H_

#include <optional>
#include <string>

#include "message.h"

Json BuildToolsSchema();
std::optional<ToolMessage> ExecuteToolCall(const Json& tool_call,
                                           std::string* error);

#endif  // TOOLS_H_
