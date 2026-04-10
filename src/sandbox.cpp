#include "sandbox.h"

#include <fmt/color.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace {

std::string Canonicalize(const std::string& path) {
  std::error_code ec;
  auto canonical = std::filesystem::canonical(path, ec);
  if (ec) {
    // If the path doesn't exist yet, try to resolve its parent.
    auto parent = std::filesystem::path(path).parent_path();
    if (parent.empty()) {
      parent = std::filesystem::current_path();
    }
    auto canonical_parent = std::filesystem::canonical(parent, ec);
    if (ec) {
      return path;
    }
    return (canonical_parent / std::filesystem::path(path).filename()).string();
  }
  return canonical.string();
}

Sandbox::Answer DefaultConfirm(const std::string& description,
                               const std::string& context) {
  fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "\n[Sandbox] ");
  fmt::print("{}\n", description);
  if (!context.empty()) {
    fmt::print("  {}\n", context);
  }
  fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold,
             "Allow this operation? [yes/no/always] ");

  std::string answer;
  std::getline(std::cin, answer);

  if (answer == "always" || answer == "ALWAYS" || answer == "a" ||
      answer == "A") {
    return Sandbox::Answer::kAlways;
  }
  if (answer == "yes" || answer == "YES" || answer == "y" || answer == "Y") {
    return Sandbox::Answer::kYes;
  }
  return Sandbox::Answer::kNo;
}

// Trim leading/trailing whitespace.
std::string Trim(const std::string& s) {
  size_t start = s.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    return "";
  }
  size_t end = s.find_last_not_of(" \t\n\r");
  return s.substr(start, end - start + 1);
}

}  // namespace

std::unordered_set<std::string> Sandbox::DefaultAllowedCommands() {
  return {
      // File inspection
      "ls",
      "cat",
      "head",
      "tail",
      "wc",
      "file",
      "stat",
      "find",
      "tree",
      // Search
      "grep",
      "rg",
      "ag",
      "awk",
      "sed",
      // Navigation / info
      "echo",
      "pwd",
      "which",
      "whoami",
      "env",
      "printenv",
      "date",
      "uname",
      // Git
      "git",
      // Build tools
      "make",
      "cmake",
      "gcc",
      "g++",
      "clang",
      "clang++",
      "rustc",
      "cargo",
      "npm",
      "node",
      "python",
      "python3",
      "pip",
      "pip3",
      // File manipulation (non-destructive)
      "mkdir",
      "cp",
      "mv",
      "touch",
      "diff",
      "patch",
      // Text processing
      "sort",
      "uniq",
      "cut",
      "tr",
      "tee",
      "xargs",
      // Process info
      "ps",
      "top",
      "htop",
      // Archive
      "tar",
      "zip",
      "unzip",
      "gzip",
      "gunzip",
  };
}

Sandbox::Sandbox(const std::string& allowed_dir)
    : allowed_dir_(Canonicalize(allowed_dir)),
      allowed_commands_(DefaultAllowedCommands()),
      confirm_fn_(DefaultConfirm) {}

Sandbox::Sandbox()
    : allowed_dir_(
          std::filesystem::canonical(std::filesystem::current_path()).string()),
      allowed_commands_(DefaultAllowedCommands()),
      confirm_fn_(DefaultConfirm) {}

// --- File path sandbox ---

bool Sandbox::IsPathAllowed(const std::string& path) const {
  const std::string resolved = Canonicalize(path);

  if (resolved.size() < allowed_dir_.size()) {
    return false;
  }
  if (resolved.compare(0, allowed_dir_.size(), allowed_dir_) != 0) {
    return false;
  }
  // Ensure it's a proper prefix (not just a substring of a longer dir name).
  if (resolved.size() > allowed_dir_.size() &&
      resolved[allowed_dir_.size()] != '/') {
    return false;
  }
  return true;
}

bool Sandbox::CheckPathOrAsk(const std::string& path) {
  if (allow_all_paths_ || IsPathAllowed(path)) {
    return true;
  }

  const Answer answer =
      confirm_fn_("Path '" + path + "' is outside the allowed directory.",
                  "Allowed directory: " + allowed_dir_);
  if (answer == Answer::kAlways) {
    allow_all_paths_ = true;
    return true;
  }
  return answer == Answer::kYes;
}

// --- Shell command allowlist ---

std::vector<std::string> Sandbox::ParseCommandNames(
    const std::string& command) {
  std::vector<std::string> names;
  std::string remaining = command;

  // Split on shell operators: ||, &&, |, ;
  // We process left to right, finding the earliest delimiter.
  while (!remaining.empty()) {
    // Find the earliest delimiter.
    size_t best = std::string::npos;
    size_t delim_len = 0;
    const std::string delimiters[] = {"||", "&&", "|", ";"};

    for (const auto& d : delimiters) {
      size_t pos = remaining.find(d);
      if (pos < best) {
        best = pos;
        delim_len = d.size();
      }
    }

    std::string segment;
    if (best == std::string::npos) {
      segment = remaining;
      remaining.clear();
    } else {
      segment = remaining.substr(0, best);
      remaining = remaining.substr(best + delim_len);
    }

    segment = Trim(segment);
    if (segment.empty()) {
      continue;
    }

    // Handle leading env vars like VAR=val cmd args...
    // and subshell prefixes like $(...)
    std::istringstream iss(segment);
    std::string token;
    std::string cmd_name;
    while (iss >> token) {
      // Skip env variable assignments (KEY=VALUE).
      if (token.find('=') != std::string::npos && token.find('=') != 0) {
        continue;
      }
      cmd_name = token;
      break;
    }

    if (!cmd_name.empty()) {
      // Strip path prefix: /usr/bin/grep -> grep
      size_t slash = cmd_name.rfind('/');
      if (slash != std::string::npos) {
        cmd_name = cmd_name.substr(slash + 1);
      }
      names.push_back(cmd_name);
    }
  }

  return names;
}

bool Sandbox::IsCommandAllowed(const std::string& command) const {
  const auto names = ParseCommandNames(command);
  for (const auto& name : names) {
    if (allowed_commands_.find(name) == allowed_commands_.end()) {
      return false;
    }
  }
  return true;
}

bool Sandbox::CheckCommandOrAsk(const std::string& command) {
  if (allow_all_commands_) {
    return true;
  }

  const auto names = ParseCommandNames(command);
  std::vector<std::string> denied;
  for (const auto& name : names) {
    if (allowed_commands_.find(name) == allowed_commands_.end()) {
      denied.push_back(name);
    }
  }

  if (denied.empty()) {
    return true;
  }

  // Build description of denied commands.
  std::string desc = "Command";
  if (denied.size() > 1) {
    desc += "s";
  }
  desc += " not in allowlist: ";
  for (size_t i = 0; i < denied.size(); ++i) {
    if (i > 0) {
      desc += ", ";
    }
    desc += "'" + denied[i] + "'";
  }

  const Answer answer = confirm_fn_(desc, "Full command: " + command);
  if (answer == Answer::kAlways) {
    // Add denied commands to session allowlist.
    for (const auto& name : denied) {
      allowed_commands_.insert(name);
    }
    return true;
  }
  return answer == Answer::kYes;
}
