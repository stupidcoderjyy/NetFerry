#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <asio.hpp>
#include <gtest/gtest.h>

#include "core/common/file_utils.h"
#include "core/processor/outside_processor.h"
#include "packet.pb.h"

namespace {

class OutsideProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "net_ferry_test_outside";
        std::filesystem::create_directories(tmp_dir_);
        inbox_dir_ = (tmp_dir_ / "inbox").string();
        outbox_dir_ = (tmp_dir_ / "outbox").string();
        std::filesystem::create_directories(inbox_dir_);
        std::filesystem::create_directories(outbox_dir_);
    }

    void TearDown() override {
        io_context_.stop();
        std::filesystem::remove_all(tmp_dir_);
    }

    std::filesystem::path tmp_dir_;
    std::string inbox_dir_;
    std::string outbox_dir_;
    asio::io_context io_context_;
};

TEST_F(OutsideProcessorTest, ProcessReqFileCleansUpOnTargetUnreachable) {
    // Use an unreachable host so the HTTP request fails quickly.
    auto processor = std::make_shared<net_ferry::processor::OutsideProcessor>(
        io_context_, inbox_dir_, outbox_dir_,
        "127.0.0.1", "1", ".resp");

    std::string request_id = "outside-test-001";
    std::string const req_path = inbox_dir_ + "/" + request_id + ".req";

    gateway::RequestPacket request;
    request.set_request_id(request_id);
    request.set_method("GET");
    request.set_url("/api/test");
    request.set_body("test body");

    ASSERT_TRUE(net_ferry::common::WriteMessageToFile(req_path, request));

    processor->OnFileCreated(req_path);
    io_context_.run_for(std::chrono::milliseconds(500));

    // The .req file should be cleaned up.
    EXPECT_FALSE(std::filesystem::exists(req_path));
}

TEST_F(OutsideProcessorTest, ProcessNonExistentFileSafely) {
    auto processor = std::make_shared<net_ferry::processor::OutsideProcessor>(
        io_context_, inbox_dir_, outbox_dir_,
        "127.0.0.1", "8080", ".resp");

    processor->OnFileCreated(inbox_dir_ + "/nonexistent.req");
    io_context_.run_for(std::chrono::milliseconds(100));
}

TEST_F(OutsideProcessorTest, ProcessGarbageFileCleansUp) {
    auto processor = std::make_shared<net_ferry::processor::OutsideProcessor>(
        io_context_, inbox_dir_, outbox_dir_,
        "127.0.0.1", "8080", ".resp");

    std::string const req_path = inbox_dir_ + "/bad.req";
    {
        std::ofstream ofs(req_path, std::ios::binary);
        ofs << "not a valid protobuf";
    }

    processor->OnFileCreated(req_path);
    io_context_.run_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(std::filesystem::exists(req_path));
}

}  // namespace
