#include <chrono>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <thread>

#include <asio.hpp>
#include <gtest/gtest.h>

#include "core/common/file_utils.h"
#include "core/network/session.h"
#include "core/network/tcp_server.h"
#include "packet.pb.h"

namespace {

class SessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() /
                   "net_ferry_test_session";
        std::filesystem::create_directories(tmp_dir_);
        outbox_dir_ = (tmp_dir_ / "outbox").string();
        std::filesystem::create_directories(outbox_dir_);
    }

    void TearDown() override {
        io_context_.stop();
        std::filesystem::remove_all(tmp_dir_);
    }

    // Send raw HTTP and close immediately without waiting for response.
    static void SendHttpAndClose(const std::string& host,
            const std::string& port,
            const std::string& request) {
        asio::io_context ctx;
        using asio::ip::tcp;
        tcp::socket socket(ctx);
        tcp::resolver resolver(ctx);
        asio::connect(socket, resolver.resolve(host, port));
        asio::write(socket, asio::buffer(request));
        std::error_code ignored;
        socket.close(ignored);
    }

    std::filesystem::path tmp_dir_;
    std::string outbox_dir_;
    asio::io_context io_context_;
};

TEST_F(SessionTest, TcpServerAcceptsConnectionAndCreatesReqFile) {
    auto server = std::make_shared<net_ferry::network::TcpServer>(
            io_context_, 0, outbox_dir_, ".req");
    server->Start();
    int const port = server->acceptor().local_endpoint().port();

    std::thread io_thread([this]() { io_context_.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    SendHttpAndClose("127.0.0.1", std::to_string(port),
            "GET /api/test HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: 0\r\n"
            "\r\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    io_context_.stop();
    io_thread.join();

    bool found_req = false;
    for (const auto& entry :
            std::filesystem::directory_iterator(outbox_dir_)) {
        if (entry.path().extension() == ".req") {
            found_req = true;
            gateway::RequestPacket packet;
            ASSERT_TRUE(net_ferry::common::ReadMessageFromFile(
                    entry.path().string(), packet));
            EXPECT_EQ(packet.method(), "GET");
            EXPECT_EQ(packet.url(), "/api/test");
            EXPECT_EQ(packet.headers().at("Host"), "localhost");
            break;
        }
    }
    EXPECT_TRUE(found_req) << "No .req file was created in outbox_dir";
}

TEST_F(SessionTest, TcpServerRequestWithBody) {
    auto server = std::make_shared<net_ferry::network::TcpServer>(
            io_context_, 0, outbox_dir_, ".req");
    server->Start();
    int const port = server->acceptor().local_endpoint().port();

    std::thread io_thread([this]() { io_context_.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string const body = R"({"key":"value"})";
    std::string const request =
            "POST /api/data HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " +
            std::to_string(body.size()) + "\r\n\r\n" + body;

    SendHttpAndClose("127.0.0.1", std::to_string(port), request);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    io_context_.stop();
    io_thread.join();

    bool found_req = false;
    for (const auto& entry :
            std::filesystem::directory_iterator(outbox_dir_)) {
        if (entry.path().extension() == ".req") {
            found_req = true;
            gateway::RequestPacket packet;
            ASSERT_TRUE(net_ferry::common::ReadMessageFromFile(
                    entry.path().string(), packet));
            EXPECT_EQ(packet.method(), "POST");
            EXPECT_EQ(packet.url(), "/api/data");
            EXPECT_EQ(packet.headers().at("Content-Type"),
                      "application/json");
            EXPECT_EQ(packet.body(), body);
            break;
        }
    }
    EXPECT_TRUE(found_req);
}

TEST_F(SessionTest, RequestIdIsUnique) {
    auto server = std::make_shared<net_ferry::network::TcpServer>(
            io_context_, 0, outbox_dir_, ".req");
    server->Start();
    int const port = server->acceptor().local_endpoint().port();

    std::thread io_thread([this]() { io_context_.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    SendHttpAndClose("127.0.0.1", std::to_string(port),
            "GET /1 HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: 0\r\n\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    SendHttpAndClose("127.0.0.1", std::to_string(port),
            "GET /2 HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: 0\r\n\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    SendHttpAndClose("127.0.0.1", std::to_string(port),
            "GET /3 HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: 0\r\n\r\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    io_context_.stop();
    io_thread.join();

    std::set<std::string> ids;
    for (const auto& entry :
            std::filesystem::directory_iterator(outbox_dir_)) {
        if (entry.path().extension() == ".req") {
            gateway::RequestPacket packet;
            ASSERT_TRUE(net_ferry::common::ReadMessageFromFile(
                    entry.path().string(), packet));
            ids.insert(packet.request_id());
        }
    }
    EXPECT_EQ(ids.size(), 3) << "Expected 3 unique request IDs";
}

}  // namespace