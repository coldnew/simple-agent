#include "sandbox.h"

#include <fmt/color.h>

#include <filesystem>
#include <iostream>

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

Sandbox::Answer DefaultConfirm(const std::string& path,
                               const std::string& allowed_dir) {
  fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "\n[Sandbox] ");
  fmt::print("Path '{}' is outside the allowed directory '{}'.\n", path,
             allowed_dir);
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

}  // namespace

Sandbox::Sandbox(const std::string& allowed_dir)
    : allowed_dir_(Canonicalize(allowed_dir)), confirm_fn_(DefaultConfirm) {}

Sandbox::Sandbox()
    : allowed_dir_(
          std::filesystem::canonical(std::filesystem::current_path()).string()),
      confirm_fn_(DefaultConfirm) {}

bool Sandbox::IsPathAllowed(const std::string& path) const {
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

bool Sandbox::CheckPathOrAsk(const std::string& path) {
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
