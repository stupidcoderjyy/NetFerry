#ifndef NETFERRY_NETWORK_SESSION_H_
#define NETFERRY_NETWORK_SESSION_H_

#include <asio.hpp>
#include <functional>
#include <memory>
#include <string>

namespace net_ferry::network {

class Session : public std::enable_shared_from_this<Session> {
public:
    using RegisterCallback = std::function<void(const std::string&, std::shared_ptr<Session>)>;

    Session(asio::io_context& io_context,
            std::string outbox_dir,
            std::string file_ext_req,
            RegisterCallback on_register);

    asio::ip::tcp::socket& socket() { return socket_; }

    void Start();

    void SendResponse(int status_code, const std::string& headers, const std::string& body);

    const std::string& request_id() const { return request_id_; }

private:
    void DoReadHeaders();
    void DoReadBody(std::size_t content_length);
    void ProcessRequest();
    void DoWrite(const std::string& response);

    static std::string GenerateRequestId();

    asio::ip::tcp::socket socket_;
    std::string outbox_dir_;
    std::string file_ext_req_;
    RegisterCallback on_register_;
    asio::streambuf request_buffer_;
    std::string request_id_;
};

}  // namespace net_ferry::network

#endif  // NETFERRY_NETWORK_SESSION_H_