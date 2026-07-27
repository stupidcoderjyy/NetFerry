#include "core/network/tcp_server.h"

#include <utility>

#include "core/common/logger.h"
#include "core/network/session.h"

namespace net_ferry::network {

TcpServer::TcpServer(
        asio::io_context& io_context, int port, std::string outbox_dir, std::string file_ext_req)
    : io_context_(io_context),
      acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      outbox_dir_(std::move(outbox_dir)),
      file_ext_req_(std::move(file_ext_req)) {}

TcpServer::~TcpServer() = default;

void TcpServer::Start() {
    DoAccept();
    LOG_INFO << "TcpServer listening on port " << acceptor_.local_endpoint().port();
}

void TcpServer::DoAccept() {
    auto session = std::make_shared<Session>(io_context_, outbox_dir_, file_ext_req_,
            [this](const std::string& request_id, std::shared_ptr<Session> s) {
        sessions_[request_id] = std::move(s);
    });
    acceptor_.async_accept(session->socket(), [this, session](const std::error_code& ec) {
        if (!ec) {
            session->Start();
        } else {
            LOG_ERROR << "Accept error: " << common::ToUtf8(ec.message());
        }
        DoAccept();
    });
}

std::shared_ptr<Session> TcpServer::FindSession(const std::string& request_id) {
    auto it = sessions_.find(request_id);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

void TcpServer::RemoveSession(const std::string& request_id) {
    sessions_.erase(request_id);
}

}  // namespace net_ferry::network
