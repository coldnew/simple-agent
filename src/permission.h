#ifndef PERMISSION_H_
#define PERMISSION_H_

#include <functional>
#include <string>

// Permission restricts file operations to a given directory.
// Paths outside the allowed directory trigger a user confirmation prompt.
class Permission {
 public:
  enum class Answer { kNo, kYes, kAlways };

  // |allowed_dir| is canonicalized at construction time.
  // If empty, uses current working directory.
  explicit Permission(const std::string& allowed_dir = "",
                      bool skip_permissions = false);

  // Returns true if |path| resolves to somewhere under |allowed_dir_|.
  bool IsPathAllowed(const std::string& path) const;

  // Checks path and, if outside allowed dir, asks the user for permission.
  // Returns true if the path is allowed or the user approved it.
  // If the user answers "always", future out-of-allowed paths are auto-allowed.
  bool CheckPathOrAsk(const std::string& path);

  const std::string& allowed_dir() const { return allowed_dir_; }

  // Override the confirmation callback (for testing).
  // Signature: Answer(const std::string& path, const std::string& allowed_dir)
  using ConfirmFn =
      std::function<Answer(const std::string&, const std::string&)>;
  void set_confirm_fn(ConfirmFn fn) { confirm_fn_ = std::move(fn); }

 private:
  std::string allowed_dir_;
  bool allow_all_ = false;
  ConfirmFn confirm_fn_;
};

#endif  // PERMISSION_H_
