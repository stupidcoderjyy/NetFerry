#include "core/network/session.h"

#include <atomic>
#include <chrono>
#include <sstream>
#include <utility>

#include "core/common/file_utils.h"
#include "core/common/logger.h"
#include "packet.pb.h"

namespace net_ferry::network {

Session::Session(asio::io_context& io_context,
        std::string outbox_dir,
        std::string file_ext_req,
        RegisterCallback on_register)
    : socket_(io_context),
      outbox_dir_(std::move(outbox_dir)),
      file_ext_req_(std::move(file_ext_req)),
      on_register_(std::move(on_register)) {}

void Session::Start() {
    DoReadHeaders();
}

void Session::DoReadHeaders() {
    auto self = shared_from_this();
    asio::async_read_until(socket_, request_buffer_, "\r\n\r\n",
            [this, self](const std::error_code& ec, std::size_t) {
        if (ec) {
            if (ec != asio::error::eof) {
                LOG_ERROR << "Session read headers error: " << common::ToUtf8(ec.message());
            }
            return;
        }

        // Determine header end position and Content-Length.
        std::size_t content_length = 0;
        std::size_t header_end = 0;
        {
            auto bufs = request_buffer_.data();
            std::string const header_block(asio::buffers_begin(bufs), asio::buffers_end(bufs));
            std::istringstream stream(header_block);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (line.empty()) {
                    header_end = static_cast<std::size_t>(stream.tellg());
                    if (std::cmp_equal(header_end, static_cast<std::size_t>(-1))) {
                        // Estimate header end.
                        header_end = header_block.find("\r\n\r\n") + 4;
                    }
                    break;
                }
                if (line.starts_with("Content-Length:") || line.starts_with("content-length:")) {
                    std::string value = line.substr(line.find(':') + 1);
                    if (!value.empty() && value.front() == ' ') {
                        value.erase(0, 1);
                    }
                    content_length = std::stoul(value);
                }
            }
        }

        // Check if body is already in the streambuf.
        std::size_t const total_bytes = request_buffer_.size();
        std::size_t const expected_total = header_end + content_length;

        if (total_bytes >= expected_total) {
            // Body already buffered.
            ProcessRequest();
        } else if (content_length > 0) {
            // Need to read more body bytes.
            DoReadBody(content_length - (total_bytes - header_end));
        } else {
            ProcessRequest();
        }
    });
}

void Session::DoReadBody(std::size_t remaining) {
    auto self = shared_from_this();
    asio::async_read(socket_, request_buffer_, asio::transfer_exactly(remaining),
            [this, self](const std::error_code& ec, std::size_t) {
        if (ec) {
            LOG_ERROR << "Session read body error: " << common::ToUtf8(ec.message());
            return;
        }
        ProcessRequest();
    });
}

void Session::ProcessRequest() {
    std::istream is(&request_buffer_);
    std::string line;

    // Parse request line.
    std::string request_line;
    std::getline(is, request_line);
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::string method;
    std::string url;
    {
        std::istringstream rl(request_line);
        rl >> method;
        rl >> url;
    }

    if (method.empty()) {
        LOG_ERROR << "Failed to parse HTTP request line: '" << request_line << "'";
        return;
    }

    gateway::RequestPacket packet;
    packet.set_method(method);
    packet.set_url(url);

    // Parse headers.
    while (std::getline(is, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            if (!value.empty() && value.front() == ' ') {
                value.erase(0, 1);
            }
            (*packet.mutable_headers())[key] = value;
        }
    }

    // Read remaining data as body.
    std::ostringstream body_oss;
    body_oss << is.rdbuf();
    std::string body = body_oss.str();
    if (!body.empty()) {
        packet.set_body(body);
    }

    request_id_ = GenerateRequestId();
    packet.set_request_id(request_id_);

    std::string const filepath = outbox_dir_ + "/" + request_id_ + file_ext_req_;
    if (!common::WriteMessageToFile(filepath, packet)) {
        LOG_ERROR << "Failed to write request file: " << filepath;
        return;
    }

    LOG_INFO << filepath << " ← " << packet.method() << " " << packet.url() << " ["
             << packet.body().size() << "B]";
    on_register_(request_id_, shared_from_this());
}

std::string Session::GenerateRequestId() {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    return std::to_string(ts) + "-" + std::to_string(++counter);
}

void Session::SendResponse(int status_code, const std::string& headers, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " OK\r\n";
    response << headers;
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    DoWrite(response.str());
}

void Session::DoWrite(const std::string& response) {
    auto self = shared_from_this();
    asio::async_write(
            socket_, asio::buffer(response), [this, self](const std::error_code& ec, std::size_t) {
        if (ec) {
            LOG_ERROR << "Session write error: " << common::ToUtf8(ec.message());
        }
        std::error_code ignored;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    });
}

}  // namespace net_ferry::network
