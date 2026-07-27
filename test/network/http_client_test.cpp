#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include <asio.hpp>
#include <gtest/gtest.h>

#include "core/common/file_utils.h"
#include "core/network/http_client.h"
#include "packet.pb.h"

namespace {

// A minimal HTTP echo server that responds to one request.
class EchoServer {
public:
    EchoServer() : acceptor_(io_context_) {}

    int Start() {
        acceptor_.open(asio::ip::tcp::v4());
        acceptor_.bind(
                asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
        acceptor_.listen();
        int const port = acceptor_.local_endpoint().port();
        AcceptOne();
        thread_ = std::thread([this]() { io_context_.run(); });
        return port;
    }

    void Stop() {
        io_context_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    void AcceptOne() {
        auto sock = std::make_shared<asio::ip::tcp::socket>(
                io_context_);
        acceptor_.async_accept(*sock, [this, sock](const std::error_code& ec) {
            if (!ec) {
                HandleClient(std::move(*sock));
            }
        });
    }

    static void HandleClient(asio::ip::tcp::socket sock) {
        auto socket = std::make_shared<asio::ip::tcp::socket>(std::move(sock));
        auto buf = std::make_shared<asio::streambuf>();
        asio::async_read_until(
                *socket, *buf, "\r\n\r\n", [socket, buf](const std::error_code& ec, std::size_t) {
            if (ec) {
                return;
            }
            std::string resp =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 13\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "Hello, World!";
            asio::async_write(*socket,
                    asio::buffer(resp),
                    [socket](const std::error_code&,
                            std::size_t) {
                std::error_code ignored;
                socket->shutdown(
                        asio::ip::tcp::socket::shutdown_both,
                        ignored);
                socket->close(ignored);
            });
        });
        // socket stays alive in the lambda capture.
    }

    asio::io_context io_context_;
    asio::ip::tcp::acceptor acceptor_;
    std::thread thread_;
};

class HttpClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() /
                   "net_ferry_test_httpclient";
        std::filesystem::create_directories(tmp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    std::filesystem::path tmp_dir_;
};

TEST_F(HttpClientTest, SendGetRequestToEchoServer) {
    EchoServer echo;
    int const port = echo.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    asio::io_context io_context;
    auto result = net_ferry::network::SendHttpRequest(io_context,
            "127.0.0.1", std::to_string(port), "GET", "/echo",
            "", "");

    echo.Stop();

    EXPECT_EQ(result.status_code, 200);
    EXPECT_EQ(result.body, "Hello, World!");
}

TEST_F(HttpClientTest, SendPostRequestWithBody) {
    EchoServer echo;
    int const port = echo.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    asio::io_context io_context;
    std::string const body = "test body content";
    auto result = net_ferry::network::SendHttpRequest(io_context,
            "127.0.0.1", std::to_string(port), "POST", "/api",
            "Content-Type: text/plain\r\n", body);

    echo.Stop();

    EXPECT_EQ(result.status_code, 200);
}

TEST_F(HttpClientTest, ConnectionRefusedReturnsZeroStatusCode) {
    asio::io_context io_context;
    auto result = net_ferry::network::SendHttpRequest(io_context,
            "127.0.0.1", "1", "GET", "/", "", "");

    EXPECT_EQ(result.status_code, 0);
}

}  // namespace