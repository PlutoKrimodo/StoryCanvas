#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "utils/FileUtils.h"

namespace fs = std::filesystem;

// ==================== 路径拼接 ====================

TEST(FileUtilsTest, JoinPathNormalizes) {
    EXPECT_EQ(FileUtils::joinPath("/tmp/storage", "users/a.png"), "/tmp/storage/users/a.png");
    // 归一化掉多余的 `.`
    EXPECT_EQ(FileUtils::joinPath("/tmp/storage", "./users/a.png"), "/tmp/storage/users/a.png");
}

TEST(FileUtilsTest, JoinPathWithEmptyBase) {
    EXPECT_EQ(FileUtils::joinPath("", "users/a.png"), "users/a.png");
}

// ==================== 路径安全（防穿越） ====================

TEST(FileUtilsTest, RejectsPathTraversal) {
    EXPECT_FALSE(FileUtils::isSafeRelativePath("../etc/passwd"));
    EXPECT_FALSE(FileUtils::isSafeRelativePath("users/../../secret"));
    EXPECT_FALSE(FileUtils::isSafeRelativePath("/etc/passwd"));
    EXPECT_FALSE(FileUtils::isSafeRelativePath("users//a.png"));
    EXPECT_FALSE(FileUtils::isSafeRelativePath("users\\a.png"));
    EXPECT_FALSE(FileUtils::isSafeRelativePath(""));
    EXPECT_FALSE(FileUtils::isSafeRelativePath("./a.png"));
}

TEST(FileUtilsTest, AcceptsNormalRelativePath) {
    EXPECT_TRUE(FileUtils::isSafeRelativePath("users/u1/books/b1/images/generated/x.png"));
    EXPECT_TRUE(FileUtils::isSafeRelativePath("a.png"));
}

// ==================== 扩展名与 MIME ====================

TEST(FileUtilsTest, ExtensionIsLowercased) {
    EXPECT_EQ(FileUtils::extensionOf("A.PNG"), "png");
    EXPECT_EQ(FileUtils::extensionOf("photo.JPEG"), "jpeg");
    EXPECT_EQ(FileUtils::extensionOf("noext"), "");
    EXPECT_EQ(FileUtils::extensionOf("trailing."), "");
}

TEST(FileUtilsTest, MimeTypeMapping) {
    EXPECT_EQ(FileUtils::mimeTypeOf("png"), "image/png");
    EXPECT_EQ(FileUtils::mimeTypeOf("jpg"), "image/jpeg");
    EXPECT_EQ(FileUtils::mimeTypeOf("webp"), "image/webp");
    EXPECT_EQ(FileUtils::mimeTypeOf("bin"), "application/octet-stream");
}

// ==================== 文件读写 ====================

TEST(FileUtilsTest, ReadWriteAndRemove) {
    const fs::path dir = fs::temp_directory_path() / "storycanvas-fileutils-test";
    fs::create_directories(dir);
    const fs::path file = dir / "sample.bin";

    {
        std::ofstream out(file, std::ios::binary);
        out << "hello";
    }

    EXPECT_EQ(FileUtils::fileSize(file.string()), 5);

    std::string content;
    ASSERT_TRUE(FileUtils::readFile(file.string(), content));
    EXPECT_EQ(content, "hello");

    EXPECT_TRUE(FileUtils::removeFile(file.string()));
    EXPECT_FALSE(fs::exists(file));
    EXPECT_EQ(FileUtils::fileSize(file.string()), -1);

    fs::remove_all(dir);
}

TEST(FileUtilsTest, EnsureParentDirCreatesDirectories) {
    const fs::path dir = fs::temp_directory_path() / "storycanvas-fileutils-parent";
    fs::remove_all(dir);

    const fs::path nested = dir / "a" / "b" / "c.png";
    EXPECT_TRUE(FileUtils::ensureParentDir(nested.string()));
    EXPECT_TRUE(fs::exists(dir / "a" / "b"));

    fs::remove_all(dir);
}
