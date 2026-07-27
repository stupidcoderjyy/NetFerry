#ifndef NETFERRY_PROCESSOR_INSIDEPROCESSOR_H_
#define NETFERRY_PROCESSOR_INSIDEPROCESSOR_H_

#include <asio.hpp>
#include <memory>
#include <string>

#include "core/network/tcp_server.h"

namespace net_ferry::processor {

class InsideProcessor {
public:
    InsideProcessor(asio::io_context& io_context,
            std::shared_ptr<network::TcpServer> server,
            std::string inbox_dir);

    void OnFileCreated(const std::string& filepath);

private:
    void ProcessRespFile(const std::string& filepath);

    asio::io_context& io_context_;
    std::shared_ptr<network::TcpServer> server_;
    std::string inbox_dir_;
};

}  // namespace net_ferry::processor

#endif  // NETFERRY_PROCESSOR_INSIDEPROCESSOR_H_
