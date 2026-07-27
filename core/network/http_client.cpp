#include "core/network/http_client.h"

#include <asio/connect.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <sstream>

#include "core/common/logger.h"

namespace net_ferry::network {

using asio::ip::tcp;

HttpResponse SendHttpRequest(asio::io_context& io_context,
        const std::string& host,
        const std::string& port,
        const std::string& method,
        const std::string& path,
        const std::string& req_headers,
        const std::string& body) {
    HttpResponse result;
    tcp::resolver resolver(io_context);
    tcp::socket socket(io_context);

    std::error_code ec;
    auto endpoints = resolver.resolve(host, port, ec);
    if (ec) {
        LOG_ERROR << "DNS resolve failed for " << host << ":" << port << " - " << ec.message();
        return result;
    }
    asio::connect(socket, endpoints, ec);
    if (ec) {
        LOG_ERROR << "Connection failed: " << ec.message();
        return result;
    }

    std::ostringstream request;
    request << method << " " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    request << req_headers;
    if (!body.empty()) {
        request << "Content-Length: " << body.size() << "\r\n";
    }
    request << "Connection: close\r\n";
    request << "\r\n";
    request << body;

    asio::write(socket, asio::buffer(request.str()), ec);
    if (ec) {
        LOG_ERROR << "Write to target failed: " << ec.message();
        return result;
    }

    asio::streambuf response_buf;
    asio::read_until(socket, response_buf, "\r\n\r\n", ec);
    if (ec && ec != asio::error::eof) {
        LOG_ERROR << "Read headers failed: " << ec.message();
        return result;
    }

    std::istream response_stream(&response_buf);
    std::string status_line;
    std::getline(response_stream, status_line);
    if (!status_line.empty() && status_line.back() == '\r') {
        status_line.pop_back();
    }

    {
        std::istringstream sl(status_line);
        std::string http_ver;
        sl >> http_ver >> result.status_code;
    }

    std::ostringstream headers_oss;
    std::string header_line;
    while (std::getline(response_stream, header_line)) {
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        if (header_line.empty()) {
            break;
        }
        headers_oss << header_line << "\r\n";
    }
    result.headers = headers_oss.str();

    // Read any remaining body data from the streambuf.
    std::ostringstream body_oss;
    if (response_buf.size() > 0) {
        body_oss << &response_buf;
    }
    // Read more from socket until EOF.
    std::error_code ignored;
    while (asio::read(socket, response_buf, asio::transfer_at_least(1), ignored) != 0U) {
        body_oss << &response_buf;
    }
    result.body = body_oss.str();

    socket.close(ignored);
    return result;
}

}  // namespace net_ferry::network
