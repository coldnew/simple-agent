#include "sandbox.cpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace {

std::filesystem::path TempDir() {
  return std::filesystem::temp_directory_path() / "sandbox_unittest";
}

struct SandboxTest : testing::Test {
  void SetUp() override {
    dir_ = TempDir();
    std::filesystem::create_directories(dir_);
    std::filesystem::create_directories(dir_ / "subdir");

    // Create test files.
    {
      std::ofstream(dir_ / "file.txt") << "hello";
    }
    {
      std::ofstream(dir_ / "subdir" / "nested.txt") << "nested";
    }
  }

  void TearDown() override { std::filesystem::remove_all(dir_); }

  std::filesystem::path dir_;
};

TEST_F(SandboxTest, AllowsPathInsideSandbox) {
  Sandbox sb(dir_.string());
  EXPECT_TRUE(sb.IsPathAllowed((dir_ / "file.txt").string()));
}

TEST_F(SandboxTest, AllowsNestedPathInsideSandbox) {
  Sandbox sb(dir_.string());
  EXPECT_TRUE(sb.IsPathAllowed((dir_ / "subdir" / "nested.txt").string()));
}

TEST_F(SandboxTest, AllowsSandboxDirItself) {
  Sandbox sb(dir_.string());
  EXPECT_TRUE(sb.IsPathAllowed(dir_.string()));
}

TEST_F(SandboxTest, DeniesPathOutsideSandbox) {
  Sandbox sb(dir_.string());
  EXPECT_FALSE(sb.IsPathAllowed("/etc/passwd"));
}

TEST_F(SandboxTest, DeniesParentTraversal) {
  Sandbox sb(dir_.string());
  EXPECT_FALSE(sb.IsPathAllowed((dir_ / ".." / "..").string()));
}

TEST_F(SandboxTest, DeniesSimilarPrefixDir) {
  // A directory like /tmp/sandbox_unittest_evil should NOT match
  // /tmp/sandbox_unittest.
  auto evil_dir = TempDir().string() + "_evil";
  std::filesystem::create_directories(evil_dir);
  {
    std::ofstream(evil_dir + "/hack.txt") << "evil";
  }

  Sandbox sb(dir_.string());
  EXPECT_FALSE(sb.IsPathAllowed(evil_dir + "/hack.txt"));

  std::filesystem::remove_all(evil_dir);
}

TEST_F(SandboxTest, CheckPathOrAskReturnsTrue_WhenInsideSandbox) {
  Sandbox sb(dir_.string());
  EXPECT_TRUE(sb.CheckPathOrAsk((dir_ / "file.txt").string()));
}

TEST_F(SandboxTest, CheckPathOrAskCallsConfirmFn_WhenOutside) {
  Sandbox sb(dir_.string());

  bool called = false;
  sb.set_confirm_fn(
      [&](const std::string&, const std::string&) -> Sandbox::Answer {
        called = true;
        return Sandbox::Answer::kYes;
      });

  EXPECT_TRUE(sb.CheckPathOrAsk("/etc/passwd"));
  EXPECT_TRUE(called);
}

TEST_F(SandboxTest, CheckPathOrAskReturnsFalse_WhenUserDenies) {
  Sandbox sb(dir_.string());

  sb.set_confirm_fn(
      [](const std::string&, const std::string&) -> Sandbox::Answer {
        return Sandbox::Answer::kNo;
      });

  EXPECT_FALSE(sb.CheckPathOrAsk("/etc/passwd"));
}

TEST_F(SandboxTest, CheckPathOrAskAlwaysSkipsFuturePrompts) {
  Sandbox sb(dir_.string());

  int call_count = 0;
  sb.set_confirm_fn(
      [&](const std::string&, const std::string&) -> Sandbox::Answer {
        ++call_count;
        return Sandbox::Answer::kAlways;
      });

  EXPECT_TRUE(sb.CheckPathOrAsk("/etc/passwd"));
  EXPECT_TRUE(sb.CheckPathOrAsk("/etc/hostname"));
  // Only the first call should have triggered the confirm fn.
  EXPECT_EQ(call_count, 1);
}

TEST_F(SandboxTest, AllowsNonExistentFileInsideSandbox) {
  Sandbox sb(dir_.string());
  // File doesn't exist yet but parent dir is inside sandbox.
  EXPECT_TRUE(sb.IsPathAllowed((dir_ / "new_file.txt").string()));
}

}  // namespace
