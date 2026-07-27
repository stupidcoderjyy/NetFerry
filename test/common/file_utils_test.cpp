#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "core/common/file_utils.h"
#include "packet.pb.h"

namespace {

class FileUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "net_ferry_test_file";
        std::filesystem::create_directories(tmp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    std::filesystem::path tmp_dir_;
};

TEST_F(FileUtilsTest, WriteAndReadRequestPacket) {
    gateway::RequestPacket original;
    original.set_request_id("test-001");
    original.set_method("POST");
    original.set_url("/api/test");
    (*original.mutable_headers())["Content-Type"] = "application/json";
    original.set_body("hello world");

    auto filepath = (tmp_dir_ / "test.req").string();

    ASSERT_TRUE(net_ferry::common::WriteMessageToFile(filepath, original));

    gateway::RequestPacket restored;
    ASSERT_TRUE(net_ferry::common::ReadMessageFromFile(filepath, restored));

    EXPECT_EQ(restored.request_id(), "test-001");
    EXPECT_EQ(restored.method(), "POST");
    EXPECT_EQ(restored.url(), "/api/test");
    EXPECT_EQ(restored.headers().at("Content-Type"), "application/json");
    EXPECT_EQ(restored.body(), "hello world");
}

TEST_F(FileUtilsTest, WriteAndReadResponsePacket) {
    gateway::ResponsePacket original;
    original.set_request_id("resp-001");
    original.set_status_code(200);
    (*original.mutable_headers())["Server"] = "nginx";
    original.set_body("OK");

    auto filepath = (tmp_dir_ / "test.resp").string();

    ASSERT_TRUE(net_ferry::common::WriteMessageToFile(filepath, original));

    gateway::ResponsePacket restored;
    ASSERT_TRUE(net_ferry::common::ReadMessageFromFile(filepath, restored));

    EXPECT_EQ(restored.request_id(), "resp-001");
    EXPECT_EQ(restored.status_code(), 200);
    EXPECT_EQ(restored.headers().at("Server"), "nginx");
    EXPECT_EQ(restored.body(), "OK");
}

TEST_F(FileUtilsTest, ReadNonExistentFileFails) {
    gateway::RequestPacket packet;
    EXPECT_FALSE(
        net_ferry::common::ReadMessageFromFile("/nonexistent/file.req", packet));
}

TEST_F(FileUtilsTest, ReadGarbageFileFails) {
    auto filepath = (tmp_dir_ / "garbage.bin").string();
    {
        std::ofstream ofs(filepath, std::ios::binary);
        ofs << "this is not a protobuf";
    }

    gateway::RequestPacket packet;
    EXPECT_FALSE(net_ferry::common::ReadMessageFromFile(filepath, packet));
}

TEST_F(FileUtilsTest, DeleteFileRemovesFile) {
    auto filepath = (tmp_dir_ / "to_delete.txt").string();
    {
        std::ofstream ofs(filepath);
        ofs << "data";
    }

    EXPECT_TRUE(std::filesystem::exists(filepath));
    EXPECT_TRUE(net_ferry::common::RemoveFile(filepath));
    EXPECT_FALSE(std::filesystem::exists(filepath));
}

TEST_F(FileUtilsTest, DeleteNonExistentFileReturnsTrue) {
    EXPECT_TRUE(
        net_ferry::common::RemoveFile((tmp_dir_ / "nonexistent.txt").string()));
}

}  // namespace
