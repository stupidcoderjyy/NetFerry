#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include <asio.hpp>
#include <gtest/gtest.h>

#include "core/common/file_utils.h"
#include "core/network/tcp_server.h"
#include "core/processor/inside_processor.h"
#include "packet.pb.h"

namespace {

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() /
                   "net_ferry_test_integration";
        std::filesystem::create_directories(tmp_dir_);
        shared_dir_ = (tmp_dir_ / "shared").string();
        std::filesystem::create_directories(shared_dir_);
        // Ensure clean state.
        for (const auto& entry : std::filesystem::directory_iterator(shared_dir_)) {
            std::filesystem::remove(entry.path());
        }
    }

    void TearDown() override {
        inside_io_.stop();
        std::filesystem::remove_all(tmp_dir_);
    }

    std::filesystem::path tmp_dir_;
    std::string shared_dir_;
    asio::io_context inside_io_;
};

// Test: InsideProcessor delivers response back to client through Session.
TEST_F(IntegrationTest, InsideProcessorReturnsResponseToClient) {
    auto server = std::make_shared<net_ferry::network::TcpServer>(
            inside_io_, 0, shared_dir_, ".req");
    server->Start();
    int const listen_port = server->acceptor().local_endpoint().port();

    auto inside_proc =
            std::make_shared<net_ferry::processor::InsideProcessor>(
                    inside_io_, server, shared_dir_);

    // Connect client and send request synchronously.
    asio::ip::tcp::socket client_socket(inside_io_);
    asio::ip::tcp::resolver resolver(inside_io_);
    asio::connect(client_socket,
            resolver.resolve("127.0.0.1",
                    std::to_string(listen_port)));

    std::string request = "GET /hello HTTP/1.1\r\n"
                          "Host: localhost\r\n"
                          "Content-Length: 0\r\n"
                          "\r\n";
    asio::write(client_socket, asio::buffer(request));

    // Process the server-side events (accept + read + write req file).
    inside_io_.run_for(std::chrono::milliseconds(500));

    // Find the .req file and extract request_id.
    std::string request_id;
    for (const auto& entry :
            std::filesystem::directory_iterator(shared_dir_)) {
        if (entry.path().extension() == ".req") {
            gateway::RequestPacket packet;
            if (net_ferry::common::ReadMessageFromFile(
                        entry.path().string(), packet)) {
                request_id = packet.request_id();
            }
            break;
        }
    }
    ASSERT_FALSE(request_id.empty());

    // Write a .resp file (simulating outside gateway response).
    gateway::ResponsePacket response;
    response.set_request_id(request_id);
    response.set_status_code(200);
    (*response.mutable_headers())["Content-Type"] = "text/plain";
    response.set_body("integration test response");

    std::string const resp_path = shared_dir_ + "/" + request_id + ".resp";
    ASSERT_TRUE(net_ferry::common::WriteMessageToFile(
            resp_path, response));

    // Trigger the inside processor.
    inside_proc->OnFileCreated(resp_path);

    // Process the response delivery (read .resp + send to client).
    inside_io_.run_for(std::chrono::milliseconds(500));

    // Read the response from the client socket.
    asio::streambuf response_buf;
    std::error_code ec;
    asio::read(client_socket, response_buf,
            asio::transfer_all(), ec);

    std::istream is(&response_buf);
    std::string status_line;
    std::getline(is, status_line);

    int status = 0;
    {
        std::istringstream sl(status_line);
        std::string http_ver;
        sl >> http_ver >> status;
    }
    EXPECT_EQ(status, 200);

    std::ostringstream body;
    body << is.rdbuf();
    EXPECT_NE(body.str().find("integration test response"),
              std::string::npos);

    std::error_code ignored;
    client_socket.close(ignored);
}

}  // namespace