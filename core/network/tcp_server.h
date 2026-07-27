#ifndef NETFERRY_NETWORK_TCPSERVER_H_
#define NETFERRY_NETWORK_TCPSERVER_H_

#include <asio.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace net_ferry::network {

class Session;

class TcpServer {
public:
    TcpServer(asio::io_context& io_context,
            int port,
            std::string outbox_dir,
            std::string file_ext_req);

    ~TcpServer();

    void Start();

    std::shared_ptr<Session> FindSession(const std::string& request_id);

    void RemoveSession(const std::string& request_id);

    const asio::ip::tcp::acceptor& acceptor() const { return acceptor_; }

private:
    void DoAccept();

    asio::io_context& io_context_;
    asio::ip::tcp::acceptor acceptor_;
    std::string outbox_dir_;
    std::string file_ext_req_;
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
};

}  // namespace net_ferry::network

#endif  // NETFERRY_NETWORK_TCPSERVER_H_
