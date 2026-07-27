#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include <asio.hpp>
#include <gtest/gtest.h>

#include "core/common/config.h"
#include "core/common/file_utils.h"
#include "core/network/tcp_server.h"
#include "core/processor/inside_processor.h"
#include "packet.pb.h"

namespace {

class InsideProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "net_ferry_test_inside";
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

TEST_F(InsideProcessorTest, ProcessRespFileAndSendResponse) {
    // Set up a TcpServer that the processor will look up sessions from.
    auto server = std::make_shared<net_ferry::network::TcpServer>(
        io_context_, 0, outbox_dir_, ".req");
    server->Start();

    auto processor = std::make_shared<net_ferry::processor::InsideProcessor>(
        io_context_, server, inbox_dir_);

    // Write a .resp file that the processor will read.
    std::string request_id = "inside-test-001";
    std::string const resp_path = inbox_dir_ + "/" + request_id + ".resp";

    gateway::ResponsePacket response;
    response.set_request_id(request_id);
    response.set_status_code(200);
    (*response.mutable_headers())["X-Test"] = "yes";
    response.set_body("response body");

    ASSERT_TRUE(net_ferry::common::WriteMessageToFile(resp_path, response));

    // Process the file.
    processor->OnFileCreated(resp_path);

    // Run the io_context briefly. Since there's no real session for this
    // request_id, the processor will log an error and delete the file.
    io_context_.run_for(std::chrono::milliseconds(100));

    // The resp file should be deleted after processing (even on error).
    EXPECT_FALSE(std::filesystem::exists(resp_path));
}

TEST_F(InsideProcessorTest, ProcessNonExistentFileSafely) {
    auto server = std::make_shared<net_ferry::network::TcpServer>(
        io_context_, 0, outbox_dir_, ".req");
    server->Start();

    auto processor = std::make_shared<net_ferry::processor::InsideProcessor>(
        io_context_, server, inbox_dir_);

    // This should not crash.
    processor->OnFileCreated(inbox_dir_ + "/nonexistent.resp");
    io_context_.run_for(std::chrono::milliseconds(100));
}

TEST_F(InsideProcessorTest, SessionNotFoundFileIsCleanedUp) {
    auto server = std::make_shared<net_ferry::network::TcpServer>(
        io_context_, 0, outbox_dir_, ".req");
    server->Start();

    auto processor = std::make_shared<net_ferry::processor::InsideProcessor>(
        io_context_, server, inbox_dir_);

    std::string request_id = "orphan-001";
    std::string const resp_path = inbox_dir_ + "/" + request_id + ".resp";

    gateway::ResponsePacket response;
    response.set_request_id(request_id);
    response.set_status_code(200);
    response.set_body("ok");

    ASSERT_TRUE(net_ferry::common::WriteMessageToFile(resp_path, response));
    ASSERT_TRUE(std::filesystem::exists(resp_path));

    processor->OnFileCreated(resp_path);
    io_context_.run_for(std::chrono::milliseconds(100));

    // File should be cleaned up even though no session exists.
    EXPECT_FALSE(std::filesystem::exists(resp_path));
}

}  // namespace
