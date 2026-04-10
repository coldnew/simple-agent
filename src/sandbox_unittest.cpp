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

// --- File path tests ---

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
  auto evil_dir = TempDir().string() + "_evil";
  std::filesystem::create_directories(evil_dir);
  {
    std::ofstream(evil_dir + "/hack.txt") << "evil";
  }

  Sandbox sb(dir_.string());
  EXPECT_FALSE(sb.IsPathAllowed(evil_dir + "/hack.txt"));

  std::filesystem::remove_all(evil_dir);
}

TEST_F(SandboxTest, AllowsNonExistentFileInsideSandbox) {
  Sandbox sb(dir_.string());
  EXPECT_TRUE(sb.IsPathAllowed((dir_ / "new_file.txt").string()));
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
  EXPECT_EQ(call_count, 1);
}

// --- Command parsing tests ---

TEST_F(SandboxTest, ParsesSingleCommand) {
  auto names = Sandbox::ParseCommandNames("ls -la");
  ASSERT_EQ(names.size(), 1);
  EXPECT_EQ(names[0], "ls");
}

TEST_F(SandboxTest, ParsesPipelineCommands) {
  auto names = Sandbox::ParseCommandNames("cat file.txt | grep foo | wc -l");
  ASSERT_EQ(names.size(), 3);
  EXPECT_EQ(names[0], "cat");
  EXPECT_EQ(names[1], "grep");
  EXPECT_EQ(names[2], "wc");
}

TEST_F(SandboxTest, ParsesAndChainedCommands) {
  auto names =
      Sandbox::ParseCommandNames("mkdir -p build && cd build && cmake ..");
  ASSERT_EQ(names.size(), 3);
  EXPECT_EQ(names[0], "mkdir");
  EXPECT_EQ(names[1], "cd");
  EXPECT_EQ(names[2], "cmake");
}

TEST_F(SandboxTest, ParsesSemicolonChainedCommands) {
  auto names = Sandbox::ParseCommandNames("echo hello; rm -rf /");
  ASSERT_EQ(names.size(), 2);
  EXPECT_EQ(names[0], "echo");
  EXPECT_EQ(names[1], "rm");
}

TEST_F(SandboxTest, ParsesOrChainedCommands) {
  auto names =
      Sandbox::ParseCommandNames("test -f foo || curl http://evil.com");
  ASSERT_EQ(names.size(), 2);
  EXPECT_EQ(names[0], "test");
  EXPECT_EQ(names[1], "curl");
}

TEST_F(SandboxTest, ParsesFullPathCommand) {
  auto names = Sandbox::ParseCommandNames("/usr/bin/grep pattern file");
  ASSERT_EQ(names.size(), 1);
  EXPECT_EQ(names[0], "grep");
}

TEST_F(SandboxTest, ParsesEnvVarPrefix) {
  auto names = Sandbox::ParseCommandNames("FOO=bar BAZ=1 git status");
  ASSERT_EQ(names.size(), 1);
  EXPECT_EQ(names[0], "git");
}

// --- Command allowlist tests ---

TEST_F(SandboxTest, AllowsWhitelistedCommand) {
  Sandbox sb(dir_.string());
  EXPECT_TRUE(sb.IsCommandAllowed("ls -la"));
  EXPECT_TRUE(sb.IsCommandAllowed("git status"));
  EXPECT_TRUE(sb.IsCommandAllowed("grep -r foo | wc -l"));
}

TEST_F(SandboxTest, DeniesNonWhitelistedCommand) {
  Sandbox sb(dir_.string());
  EXPECT_FALSE(sb.IsCommandAllowed("curl http://evil.com"));
  EXPECT_FALSE(sb.IsCommandAllowed("wget http://evil.com"));
  EXPECT_FALSE(sb.IsCommandAllowed("rm -rf /"));
}

TEST_F(SandboxTest, DeniesIfAnyCommandInPipelineNotAllowed) {
  Sandbox sb(dir_.string());
  EXPECT_FALSE(sb.IsCommandAllowed("echo hello | curl http://evil.com"));
}

TEST_F(SandboxTest, CheckCommandOrAskAllowsWhitelisted) {
  Sandbox sb(dir_.string());
  EXPECT_TRUE(sb.CheckCommandOrAsk("ls -la"));
}

TEST_F(SandboxTest, CheckCommandOrAskPromptsForDenied) {
  Sandbox sb(dir_.string());

  bool called = false;
  sb.set_confirm_fn(
      [&](const std::string& desc, const std::string&) -> Sandbox::Answer {
        called = true;
        EXPECT_NE(desc.find("curl"), std::string::npos);
        return Sandbox::Answer::kNo;
      });

  EXPECT_FALSE(sb.CheckCommandOrAsk("curl http://example.com"));
  EXPECT_TRUE(called);
}

TEST_F(SandboxTest, CheckCommandOrAskAlwaysAddsToAllowlist) {
  Sandbox sb(dir_.string());

  int call_count = 0;
  sb.set_confirm_fn(
      [&](const std::string&, const std::string&) -> Sandbox::Answer {
        ++call_count;
        return Sandbox::Answer::kAlways;
      });

  EXPECT_TRUE(sb.CheckCommandOrAsk("curl http://example.com"));
  // Same command should now be allowed without prompting.
  EXPECT_TRUE(sb.CheckCommandOrAsk("curl http://other.com"));
  EXPECT_EQ(call_count, 1);
}

}  // namespace
