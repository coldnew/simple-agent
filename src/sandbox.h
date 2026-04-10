#ifndef SANDBOX_H_
#define SANDBOX_H_

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

// Sandbox restricts file operations to a given directory and shell commands
// to an allowlist of known-safe programs.
class Sandbox {
 public:
  enum class Answer { kNo, kYes, kAlways };

  // |allowed_dir| is canonicalized at construction time.
  explicit Sandbox(const std::string& allowed_dir);

  // Default: uses current working directory.
  Sandbox();

  // --- File path sandbox ---

  // Returns true if |path| resolves to somewhere under |allowed_dir_|.
  bool IsPathAllowed(const std::string& path) const;

  // Checks path and, if outside sandbox, asks the user for permission.
  // If the user answers "always", future out-of-sandbox paths are auto-allowed.
  bool CheckPathOrAsk(const std::string& path);

  const std::string& allowed_dir() const { return allowed_dir_; }

  // --- Shell command allowlist ---

  // Returns true if every command in the pipeline is in the allowlist.
  bool IsCommandAllowed(const std::string& command) const;

  // Checks command and, if not allowed, asks the user for permission.
  // "always" adds the denied command(s) to the session allowlist.
  bool CheckCommandOrAsk(const std::string& command);

  // Extract individual command names from a shell command string.
  // Splits on |, &&, ;, || and returns the base command name of each segment.
  static std::vector<std::string> ParseCommandNames(const std::string& command);

  // --- Confirmation callback ---

  // Override the confirmation callback (for testing).
  // first arg: description of what's being asked about
  // second arg: reason/context
  using ConfirmFn =
      std::function<Answer(const std::string&, const std::string&)>;
  void set_confirm_fn(ConfirmFn fn) { confirm_fn_ = std::move(fn); }

 private:
  std::string allowed_dir_;
  bool allow_all_paths_ = false;
  bool allow_all_commands_ = false;
  std::unordered_set<std::string> allowed_commands_;
  ConfirmFn confirm_fn_;

  static std::unordered_set<std::string> DefaultAllowedCommands();
};

#endif  // SANDBOX_H_
