#include "permission.cpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace {

std::filesystem::path TempDir() {
  return std::filesystem::temp_directory_path() / "permission_unittest";
}

struct PermissionTest : testing::Test {
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

TEST_F(PermissionTest, AllowsPathInsidePermission) {
  Permission sb(dir_.string());
  EXPECT_TRUE(sb.IsPathAllowed((dir_ / "file.txt").string()));
}

TEST_F(PermissionTest, AllowsNestedPathInsidePermission) {
  Permission sb(dir_.string());
  EXPECT_TRUE(sb.IsPathAllowed((dir_ / "subdir" / "nested.txt").string()));
}

TEST_F(PermissionTest, AllowsPermissionDirItself) {
  Permission sb(dir_.string());
  EXPECT_TRUE(sb.IsPathAllowed(dir_.string()));
}

TEST_F(PermissionTest, DeniesPathOutsidePermission) {
  Permission sb(dir_.string());
  EXPECT_FALSE(sb.IsPathAllowed("/etc/passwd"));
}

TEST_F(PermissionTest, DeniesParentTraversal) {
  Permission sb(dir_.string());
  EXPECT_FALSE(sb.IsPathAllowed((dir_ / ".." / "..").string()));
}

TEST_F(PermissionTest, DeniesSimilarPrefixDir) {
  // A directory like /tmp/permission_unittest_evil should NOT match
  // /tmp/permission_unittest.
  auto evil_dir = TempDir().string() + "_evil";
  std::filesystem::create_directories(evil_dir);
  {
    std::ofstream(evil_dir + "/hack.txt") << "evil";
  }

  Permission sb(dir_.string());
  EXPECT_FALSE(sb.IsPathAllowed(evil_dir + "/hack.txt"));

  std::filesystem::remove_all(evil_dir);
}

TEST_F(PermissionTest, CheckPathOrAskReturnsTrue_WhenInsidePermission) {
  Permission sb(dir_.string());
  EXPECT_TRUE(sb.CheckPathOrAsk((dir_ / "file.txt").string()));
}

TEST_F(PermissionTest, CheckPathOrAskCallsConfirmFn_WhenOutside) {
  Permission sb(dir_.string());

  bool called = false;
  sb.set_confirm_fn(
      [&](const std::string&, const std::string&) -> Permission::Answer {
        called = true;
        return Permission::Answer::kYes;
      });

  EXPECT_TRUE(sb.CheckPathOrAsk("/etc/passwd"));
  EXPECT_TRUE(called);
}

TEST_F(PermissionTest, CheckPathOrAskReturnsFalse_WhenUserDenies) {
  Permission sb(dir_.string());

  sb.set_confirm_fn(
      [](const std::string&, const std::string&) -> Permission::Answer {
        return Permission::Answer::kNo;
      });

  EXPECT_FALSE(sb.CheckPathOrAsk("/etc/passwd"));
}

TEST_F(PermissionTest, CheckPathOrAskAlwaysSkipsFuturePrompts) {
  Permission sb(dir_.string());

  int call_count = 0;
  sb.set_confirm_fn(
      [&](const std::string&, const std::string&) -> Permission::Answer {
        ++call_count;
        return Permission::Answer::kAlways;
      });

  EXPECT_TRUE(sb.CheckPathOrAsk("/etc/passwd"));
  EXPECT_TRUE(sb.CheckPathOrAsk("/etc/hostname"));
  // Only the first call should have triggered the confirm fn.
  EXPECT_EQ(call_count, 1);
}

TEST_F(PermissionTest, AllowsNonExistentFileInsidePermission) {
  Permission sb(dir_.string());
  // File doesn't exist yet but parent dir is inside permission.
  EXPECT_TRUE(sb.IsPathAllowed((dir_ / "new_file.txt").string()));
}

}  // namespace
