#include "permission.h"

#include <fmt/color.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <unordered_set>

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

Permission::Answer DefaultConfirm(const std::string& path,
                                  const std::string& allowed_dir) {
  fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "\n[Permission] ");
  fmt::print("Path '{}' is outside the allowed directory '{}'.\n", path,
             allowed_dir);
  fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold,
             "Allow this operation? [yes/no/always] ");

  std::string answer;
  std::getline(std::cin, answer);

  if (answer == "always" || answer == "ALWAYS" || answer == "a" ||
      answer == "A") {
    return Permission::Answer::kAlways;
  }
  if (answer == "yes" || answer == "YES" || answer == "y" || answer == "Y") {
    return Permission::Answer::kYes;
  }
  return Permission::Answer::kNo;
}

// Allowed shell commands (read-only, safe operations).
const std::unordered_set<std::string> kAllowedShellCommands = {
    "ls",     "cat",  "git",  "grep", "find", "echo",  "pwd",
    "cd",     "tree", "head", "tail", "wc",   "which", "whoami",
    "ls -la", "pwd",  "date", "id",   "uname"};

// Blocked shell patterns (dangerous operations).
const std::unordered_set<std::string> kBlockedShellPatterns = {
    "rm -rf", "sudo",    "chmod 777", "|",       ";",       "&&",   "||",
    "`",      "$(",      "curl",      "wget",    "nc",      "ncat", "python",
    "node",   "ruby",    "perl",      "bash -i", "/bin/sh", "exec", "eval",
    "sh -c",  "bash -c", "kill",      "killall", "pkill",   "dd",   "mkfs",
    "mount",  "umount",  "reboot",    "shutdown"};

}  // namespace

// Static member initialization.
const std::unordered_set<std::string> Permission::kAllowedShellCommands =
    kAllowedShellCommands;
const std::unordered_set<std::string> Permission::kBlockedShellPatterns =
    kBlockedShellPatterns;

Permission::Permission(const std::string& allowed_dir, bool skip_permissions)
    : allowed_dir_(allowed_dir.empty() ? std::filesystem::canonical(
                                             std::filesystem::current_path())
                                             .string()
                                       : Canonicalize(allowed_dir)),
      allow_all_(skip_permissions),
      confirm_fn_(DefaultConfirm) {}

bool Permission::IsPathAllowed(const std::string& path) const {
  const std::string resolved = Canonicalize(path);

  // Check that resolved path starts with the allowed directory.
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

bool Permission::CheckPathOrAsk(const std::string& path) {
  if (allow_all_ || IsPathAllowed(path)) {
    return true;
  }

  const Answer answer = confirm_fn_(path, allowed_dir_);
  if (answer == Answer::kAlways) {
    allow_all_ = true;
    return true;
  }
  return answer == Answer::kYes;
}

bool Permission::CheckShellCommand(const std::string& command) {
  if (allow_all_) {
    return true;
  }

  // Check blocked patterns first.
  for (const auto& pattern : kBlockedShellPatterns) {
    if (command.find(pattern) != std::string::npos) {
      std::cerr << "[Permission] Blocked dangerous pattern: " << pattern
                << std::endl;
      return false;
    }
  }

  // Extract base command (first word).
  std::istringstream iss(command);
  std::string cmd_base;
  iss >> cmd_base;

  // Check allowlist.
  if (kAllowedShellCommands.find(cmd_base) != kAllowedShellCommands.end()) {
    return true;
  }

  // Not in allowlist - ask user for permission.
  const Answer answer = confirm_fn_("shell: " + command, allowed_dir_);
  if (answer == Answer::kAlways) {
    allow_all_ = true;
    return true;
  }
  return answer == Answer::kYes;
}
