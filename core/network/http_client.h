#ifndef NETFERRY_NETWORK_HTTPCLIENT_H_
#define NETFERRY_NETWORK_HTTPCLIENT_H_

#include <asio.hpp>
#include <string>

namespace net_ferry::network {

struct HttpResponse {
    int status_code = 0;
    std::string headers;
    std::string body;
};

HttpResponse SendHttpRequest(asio::io_context& io_context,
        const std::string& host,
        const std::string& port,
        const std::string& method,
        const std::string& path,
        const std::string& req_headers,
        const std::string& body);

}  // namespace net_ferry::network

#endif  // NETFERRY_NETWORK_HTTPCLIENT_H_
